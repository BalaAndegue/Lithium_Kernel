// ===========================================================================
// virtio_disk.c - Pilote disque VirtIO pour QEMU virt RISC-V
// ===========================================================================
//
// VirtIO utilise des "virtqueues" : des tableaux de descripteurs partagés
// entre le driver (kernel) et le périphérique (QEMU).
//
// Pour lire/écrire un bloc :
//   1. Remplir 3 descripteurs (header, data, status)
//   2. Les placer dans le driver ring (avail)
//   3. Notifier QEMU via QUEUE_NOTIFY
//   4. Dormir jusqu'à l'interruption (QEMU remplit le used ring)
//   5. Vérifier le status byte
// ===========================================================================

#include <stddef.h>
#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/fs/virtio.h"
#include "kernel/fs/virtio_defs.h"
#include "kernel/fs/buffer.h"
#include "kernel/mem/spinlock.h"
#include "kernel/mem/layout.h"
#include "kernel/proc/control.h"
#include "kernel/io/console.h"

// Accès aux registres MMIO VirtIO
#define R(reg) ((volatile uint32 *)(VIRTIO0 + (reg)))

// ---------------------------------------------------------------------------
// Structures de la virtqueue (alignées sur PAGE_SIZE)
// ---------------------------------------------------------------------------
static struct virtq_desc  __attribute__((aligned(16))) desc[VIRTIO_RING_SIZE];
static struct virtq_avail __attribute__((aligned(2)))  avail;
static struct virtq_used  __attribute__((aligned(4096))) used;

// État interne pour chaque entrée de la queue
static struct {
    struct buf *b;      // Buffer en cours de traitement
    char        status; // Rempli par le périphérique (0 = succès)
} info[VIRTIO_RING_SIZE];

// En-tête de requête partagé (un par opération)
static struct virtio_blk_req ops[VIRTIO_RING_SIZE];

static struct spinlock vdisk_lock;

// ---------------------------------------------------------------------------
// virtio_disk_init - Configurer le périphérique VirtIO disque
// ---------------------------------------------------------------------------
void virtio_disk_init(void)
{
    spinlock_init(&vdisk_lock, "virtio_disk");

    // Vérifier que c'est bien un périphérique VirtIO bloc
    if (*R(VIRTIO_MMIO_MAGIC_VALUE)  != 0x74726976 ||
        *R(VIRTIO_MMIO_VERSION)      != 2           ||
        *R(VIRTIO_MMIO_DEVICE_ID)    != 2           ||
        *R(VIRTIO_MMIO_VENDOR_ID)    != 0x554d4551) {
        printf("virtio_disk_init: périphérique non trouvé\n");
        return;
    }

    // Séquence d'initialisation VirtIO spec v1.1
    uint32 status = 0;

    *R(VIRTIO_MMIO_STATUS) = status;                           // Reset
    status |= VIRTIO_CONFIG_S_ACKNOWLEDGE;
    *R(VIRTIO_MMIO_STATUS) = status;
    status |= VIRTIO_CONFIG_S_DRIVER;
    *R(VIRTIO_MMIO_STATUS) = status;

    // Négocier les features (on accepte ce que le périphérique offre)
    uint64 features = *R(VIRTIO_MMIO_DEVICE_FEATURES);
    features &= ~(1 << VIRTIO_BLK_F_RO);
    features &= ~(1 << VIRTIO_BLK_F_SCSI);
    features &= ~(1 << VIRTIO_RING_F_EVENT_IDX);
    features &= ~(1 << VIRTIO_RING_F_INDIRECT_DESC);
    *R(VIRTIO_MMIO_DRIVER_FEATURES) = (uint32)features;

    status |= VIRTIO_CONFIG_S_FEATURES_OK;
    *R(VIRTIO_MMIO_STATUS) = status;
    status |= VIRTIO_CONFIG_S_DRIVER_OK;
    *R(VIRTIO_MMIO_STATUS) = status;

    // Configurer la virtqueue 0
    *R(VIRTIO_MMIO_QUEUE_SEL) = 0;

    if (*R(VIRTIO_MMIO_QUEUE_READY))
        printf("virtio_disk_init: queue déjà prête\n");

    uint32 max = *R(VIRTIO_MMIO_QUEUE_NUM_MAX);
    if (max == 0 || max < VIRTIO_RING_SIZE) {
        printf("virtio_disk_init: queue trop petite (max=%u)\n", max);
        return;
    }

    *R(VIRTIO_MMIO_QUEUE_NUM) = VIRTIO_RING_SIZE;

    // Fournir les adresses physiques des structures de la queue
    *R(VIRTIO_MMIO_QUEUE_DESC_LOW)   = (uint32)(uint64)desc;
    *R(VIRTIO_MMIO_QUEUE_DESC_HIGH)  = (uint32)((uint64)desc >> 32);
    *R(VIRTIO_MMIO_DRIVER_DESC_LOW)  = (uint32)(uint64)&avail;
    *R(VIRTIO_MMIO_DRIVER_DESC_HIGH) = (uint32)((uint64)&avail >> 32);
    *R(VIRTIO_MMIO_DEVICE_DESC_LOW)  = (uint32)(uint64)&used;
    *R(VIRTIO_MMIO_DEVICE_DESC_HIGH) = (uint32)((uint64)&used >> 32);

    *R(VIRTIO_MMIO_QUEUE_READY) = 1;

    printf("virtio_disk_init: disque VirtIO prêt\n");
}

// ---------------------------------------------------------------------------
// Allouer un descripteur libre dans la virtqueue
// ---------------------------------------------------------------------------
static int alloc_desc(void)
{
    for (int i = 0; i < VIRTIO_RING_SIZE; i++) {
        if (info[i].b == NULL)
            return i;
    }
    return -1;
}

static void free_desc(int i)
{
    desc[i].addr  = 0;
    desc[i].len   = 0;
    desc[i].flags = 0;
    desc[i].next  = 0;
    info[i].b     = NULL;
    wakeup(&info[i].b);
}

// ---------------------------------------------------------------------------
// virtio_disk_rw - Soumettre une requête de lecture ou d'écriture
// ---------------------------------------------------------------------------
void virtio_disk_rw(struct buf *b, int write)
{
    uint64 sector = b->blockno * (BSIZE / 512);

    spinlock_acquire(&vdisk_lock);

    // Allouer 3 descripteurs : header, data, status
    int idx[3];
    while (1) {
        int ok = 1;
        for (int i = 0; i < 3; i++) {
            idx[i] = alloc_desc();
            if (idx[i] < 0) { ok = 0; break; }
            // Marquer comme utilisé temporairement
            info[idx[i]].b = b;
        }
        if (ok) break;
        // Pas assez de descripteurs : attendre
        for (int i = 0; i < 3; i++)
            if (idx[i] >= 0) free_desc(idx[i]);
        sleep(&info[0].b, &vdisk_lock);
    }

    // Descripteur 0 : en-tête de requête
    struct virtio_blk_req *op = &ops[idx[0]];
    op->type     = write ? VIRTIO_BLK_T_OUT : VIRTIO_BLK_T_IN;
    op->reserved = 0;
    op->sector   = sector;

    desc[idx[0]].addr  = (uint64)op;
    desc[idx[0]].len   = sizeof(struct virtio_blk_req);
    desc[idx[0]].flags = VIRTQ_DESC_F_NEXT;
    desc[idx[0]].next  = (uint16)idx[1];

    // Descripteur 1 : données (le bloc de 512 octets)
    desc[idx[1]].addr  = (uint64)b->data;
    desc[idx[1]].len   = BSIZE;
    // En lecture, le périphérique écrit dans le buffer → WRITE flag
    desc[idx[1]].flags = (write ? 0 : VIRTQ_DESC_F_WRITE) | VIRTQ_DESC_F_NEXT;
    desc[idx[1]].next  = (uint16)idx[2];

    // Descripteur 2 : byte de statut (le périphérique y écrit 0 si succès)
    info[idx[0]].status = 0xff;
    desc[idx[2]].addr   = (uint64)&info[idx[0]].status;
    desc[idx[2]].len    = 1;
    desc[idx[2]].flags  = VIRTQ_DESC_F_WRITE;
    desc[idx[2]].next   = 0;

    // Associer le buffer au dernier descripteur de la chaîne
    info[idx[0]].b = b;
    b->flags      &= ~B_VALID;

    // Soumettre dans le driver ring
    avail.ring[avail.idx % VIRTIO_RING_SIZE] = (uint16)idx[0];
    __sync_synchronize();
    avail.idx++;
    __sync_synchronize();

    // Notifier QEMU
    *R(VIRTIO_MMIO_QUEUE_NOTIFY) = 0;

    // Attendre la fin de l'opération (l'interruption réveillera ce processus)
    while ((b->flags & B_VALID) == 0)
        sleep(b, &vdisk_lock);

    info[idx[0]].b = NULL;
    free_desc(idx[0]);
    free_desc(idx[1]);
    free_desc(idx[2]);

    spinlock_release(&vdisk_lock);
}

// ---------------------------------------------------------------------------
// virtio_disk_intr - Gérer l'interruption VirtIO (appelée par kerneltrap)
// ---------------------------------------------------------------------------
void virtio_disk_intr(void)
{
    spinlock_acquire(&vdisk_lock);

    // Acquitter l'interruption
    *R(VIRTIO_MMIO_INTERRUPT_ACK) = *R(VIRTIO_MMIO_INTERRUPT_STATUS) & 0x3;
    __sync_synchronize();

    // Parcourir les entrées complétées dans le used ring
    static uint16 used_idx = 0;

    while (used_idx != used.idx) {
        __sync_synchronize();
        int id = (int)used.ring[used_idx % VIRTIO_RING_SIZE].id;

        if (info[id].status != 0) {
            printf("virtio_disk_intr: erreur disque status=%d\n",
                   info[id].status);
        }

        if (info[id].b != NULL) {
            info[id].b->flags |= B_VALID;
            wakeup(info[id].b);
        }

        used_idx++;
    }

    spinlock_release(&vdisk_lock);
}
