// ===========================================================================
// buffer.c - Cache de blocs disque (buffer cache LRU)
// ===========================================================================
//
// Le buffer cache maintient une liste LRU des blocs récemment lus.
// Invariants :
//   - Un buf avec refcnt > 0 ne peut pas être évincé du cache
//   - Un buf B_DIRTY doit être écrit sur disque avant réutilisation
// ===========================================================================

#include <stddef.h>
#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/fs/buffer.h"
#include "kernel/fs/sleeplock.h"
#include "kernel/mem/spinlock.h"
#include "kernel/io/console.h"

struct bcache bcache;

// ---------------------------------------------------------------------------
// buf_cache_init - Construire la liste LRU circulaire avec head en sentinelle
// ---------------------------------------------------------------------------
void buf_cache_init(void)
{
    struct buf *b;

    spinlock_init(&bcache.lock, "bcache");

    bcache.head.prev = &bcache.head;
    bcache.head.next = &bcache.head;

    for (b = bcache.bufs; b < bcache.bufs + NBUF; b++) {
        b->next = bcache.head.next;
        b->prev = &bcache.head;
        sleeplock_init(&b->lock, "buffer");
        bcache.head.next->prev = b;
        bcache.head.next = b;
    }

    printf("buf_cache_init: %d buffers x %d octets\n", NBUF, BSIZE);
}

// ---------------------------------------------------------------------------
// buf_get - Chercher (dev, blockno) en cache ou recycler un buffer LRU libre
// ---------------------------------------------------------------------------
struct buf *buf_get(uint32 dev, uint32 blockno)
{
    struct buf *b;

    spinlock_acquire(&bcache.lock);

    // Recherche dans le cache (tête = MRU)
    for (b = bcache.head.next; b != &bcache.head; b = b->next) {
        if (b->dev == dev && b->blockno == blockno) {
            b->refcnt++;
            spinlock_release(&bcache.lock);
            sleeplock_acquire(&b->lock);
            return b;
        }
    }

    // Non trouvé : recycler le LRU non référencé (queue = LRU)
    for (b = bcache.head.prev; b != &bcache.head; b = b->prev) {
        if (b->refcnt == 0 && !(b->flags & B_DIRTY)) {
            b->dev     = dev;
            b->blockno = blockno;
            b->flags   = 0;
            b->refcnt  = 1;
            spinlock_release(&bcache.lock);
            sleeplock_acquire(&b->lock);
            return b;
        }
    }

    printf("buf_get: cache épuisé (dev=%u blockno=%u)\n", dev, blockno);
    spinlock_release(&bcache.lock);
    return NULL;
}

// ---------------------------------------------------------------------------
// buf_release - Remettre le buffer en tête LRU et décrémenter refcnt
// ---------------------------------------------------------------------------
void buf_release(struct buf *b)
{
    if (!sleeplock_held(&b->lock)) {
        printf("buf_release: buffer non verrouillé\n");
        return;
    }

    sleeplock_release(&b->lock);

    spinlock_acquire(&bcache.lock);
    b->refcnt--;

    if (b->refcnt == 0) {
        // Replacer en tête (MRU)
        b->next->prev = b->prev;
        b->prev->next = b->next;
        b->next = bcache.head.next;
        b->prev = &bcache.head;
        bcache.head.next->prev = b;
        bcache.head.next = b;
    }

    spinlock_release(&bcache.lock);
}

void buf_pin(struct buf *b)
{
    spinlock_acquire(&bcache.lock);
    b->refcnt++;
    spinlock_release(&bcache.lock);
}

void buf_unpin(struct buf *b)
{
    spinlock_acquire(&bcache.lock);
    b->refcnt--;
    spinlock_release(&bcache.lock);
}
