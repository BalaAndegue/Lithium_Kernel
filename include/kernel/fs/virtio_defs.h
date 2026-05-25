// ===========================================================================
// virtio_defs.h - Registres et constantes du protocole VirtIO (MMIO)
// ===========================================================================
// VirtIO est le standard de périphériques virtuels pour les VMs.
// Sur QEMU virt RISC-V, le disque VirtIO est accessible via MMIO à 0x10001000.
// ===========================================================================

#ifndef KERNEL_FS_VIRTIO_DEFS_H
#define KERNEL_FS_VIRTIO_DEFS_H

#include "kernel/types.h"

// Adresse de base du premier disque VirtIO sur QEMU virt
#define VIRTIO0  0x10001000L

// ---------------------------------------------------------------------------
// Registres MMIO VirtIO (offset depuis VIRTIO0)
// ---------------------------------------------------------------------------
#define VIRTIO_MMIO_MAGIC_VALUE      0x000  // Doit lire 0x74726976 ("virt")
#define VIRTIO_MMIO_VERSION          0x004  // Doit être 2
#define VIRTIO_MMIO_DEVICE_ID        0x008  // 2 = disque bloc
#define VIRTIO_MMIO_VENDOR_ID        0x00c
#define VIRTIO_MMIO_DEVICE_FEATURES  0x010
#define VIRTIO_MMIO_DRIVER_FEATURES  0x020
#define VIRTIO_MMIO_QUEUE_SEL        0x030  // Sélectionner une virtqueue
#define VIRTIO_MMIO_QUEUE_NUM_MAX    0x034  // Taille max de la queue
#define VIRTIO_MMIO_QUEUE_NUM        0x038  // Taille qu'on veut utiliser
#define VIRTIO_MMIO_QUEUE_READY      0x044  // Mettre à 1 pour activer
#define VIRTIO_MMIO_QUEUE_NOTIFY     0x050  // Notifier le périphérique
#define VIRTIO_MMIO_INTERRUPT_STATUS 0x060
#define VIRTIO_MMIO_INTERRUPT_ACK    0x064
#define VIRTIO_MMIO_STATUS           0x070  // Registre de statut
#define VIRTIO_MMIO_QUEUE_DESC_LOW   0x080  // Adresse basse des descripteurs
#define VIRTIO_MMIO_QUEUE_DESC_HIGH  0x084
#define VIRTIO_MMIO_DRIVER_DESC_LOW  0x090  // Adresse basse du driver ring
#define VIRTIO_MMIO_DRIVER_DESC_HIGH 0x094
#define VIRTIO_MMIO_DEVICE_DESC_LOW  0x0a0  // Adresse basse du device ring
#define VIRTIO_MMIO_DEVICE_DESC_HIGH 0x0a4

// Bits du registre STATUS
#define VIRTIO_CONFIG_S_ACKNOWLEDGE  1
#define VIRTIO_CONFIG_S_DRIVER       2
#define VIRTIO_CONFIG_S_DRIVER_OK    4
#define VIRTIO_CONFIG_S_FEATURES_OK  8

// Bits des features du disque bloc
#define VIRTIO_BLK_F_RO              5   // Disque en lecture seule
#define VIRTIO_BLK_F_SCSI            7   // Support SCSI
#define VIRTIO_BLK_F_CONFIG_WCE     11   // Mode writeback cache
#define VIRTIO_BLK_F_MQ             12   // Multi-queue
#define VIRTIO_F_ANY_LAYOUT         27
#define VIRTIO_RING_F_INDIRECT_DESC 28
#define VIRTIO_RING_F_EVENT_IDX     29

// Taille de la virtqueue (nombre de descripteurs)
#define VIRTIO_RING_SIZE             8

// ---------------------------------------------------------------------------
// Structures de la virtqueue
// ---------------------------------------------------------------------------

// Un descripteur pointe vers un buffer mémoire
struct virtq_desc {
    uint64 addr;   // Adresse physique du buffer
    uint32 len;    // Longueur
    uint16 flags;  // VIRTQ_DESC_F_NEXT, VIRTQ_DESC_F_WRITE
    uint16 next;   // Index du prochain descripteur (si NEXT activé)
};

#define VIRTQ_DESC_F_NEXT   1   // Ce descripteur est chaîné avec 'next'
#define VIRTQ_DESC_F_WRITE  2   // Le périphérique peut écrire dans ce buffer

// Le driver ring : indices des chaînes de descripteurs soumis
struct virtq_avail {
    uint16 flags;
    uint16 idx;
    uint16 ring[VIRTIO_RING_SIZE];
};

// Une entrée du device ring (résultat d'une opération)
struct virtq_used_elem {
    uint32 id;   // Index de la chaîne de descripteurs
    uint32 len;  // Nombre d'octets écrits par le périphérique
};

struct virtq_used {
    uint16 flags;
    uint16 idx;
    struct virtq_used_elem ring[VIRTIO_RING_SIZE];
};

// ---------------------------------------------------------------------------
// Requête disque bloc VirtIO
// ---------------------------------------------------------------------------
struct virtio_blk_req {
    uint32 type;    // VIRTIO_BLK_T_IN (lecture) ou T_OUT (écriture)
    uint32 reserved;
    uint64 sector;  // Numéro de secteur (1 secteur = 512 octets = 1 bloc)
};

#define VIRTIO_BLK_T_IN   0   // Lecture depuis le disque
#define VIRTIO_BLK_T_OUT  1   // Écriture sur le disque

#endif
