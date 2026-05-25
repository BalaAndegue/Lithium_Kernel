// ===========================================================================
// block.c - Couche bloc : interface entre le cache et le disque VirtIO
// ===========================================================================
//
// block_read  : obtient un buffer du cache (buf_get) puis, si les données
//               ne sont pas à jour (B_VALID absent), les charge depuis le disque
// block_write : marque le buffer dirty et le soumet au driver VirtIO
// ===========================================================================

#include <stddef.h>
#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/fs/block.h"
#include "kernel/fs/buffer.h"
#include "kernel/io/console.h"

// Déclarées dans virtio_disk.c
extern void virtio_disk_rw(struct buf *b, int write);

// ---------------------------------------------------------------------------
// block_read - Lire le bloc (dev, blockno) depuis le disque
// ---------------------------------------------------------------------------
struct buf *block_read(uint32 dev, uint32 blockno)
{
    struct buf *b = buf_get(dev, blockno);
    if (b == NULL) return NULL;

    if (!(b->flags & B_VALID)) {
        // Pas en cache : lire depuis le disque
        virtio_disk_rw(b, 0);
    }

    return b;
}

// ---------------------------------------------------------------------------
// block_write - Écrire le buffer modifié sur le disque
// ---------------------------------------------------------------------------
void block_write(struct buf *b)
{
    if (!sleeplock_held(&b->lock)) {
        printf("block_write: buffer non verrouillé\n");
        return;
    }

    b->flags |= B_DIRTY;
    virtio_disk_rw(b, 1);
}

// ---------------------------------------------------------------------------
// block_release - Libérer le buffer (appelle buf_release)
// ---------------------------------------------------------------------------
void block_release(struct buf *b)
{
    buf_release(b);
}

void block_pin(struct buf *b)   { buf_pin(b); }
void block_unpin(struct buf *b) { buf_unpin(b); }
