// ===========================================================================
// buffer.h - Structure du cache de blocs disque
// ===========================================================================
// Le buffer cache garde en mémoire les blocs disque récemment utilisés
// pour éviter des accès répétés au disque (lents).
// Chaque buf représente un bloc disque d'exactement BSIZE octets.
// ===========================================================================

#ifndef KERNEL_FS_BUFFER_H
#define KERNEL_FS_BUFFER_H

#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/mem/spinlock.h"
#include "kernel/fs/sleeplock.h"

// Drapeaux d'état d'un buffer
#define B_VALID   0x2   // Le buffer contient des données à jour du disque
#define B_DIRTY   0x4   // Le buffer a été modifié, pas encore écrit sur disque

struct buf {
    int            flags;        // B_VALID, B_DIRTY
    uint32         dev;          // Numéro de périphérique
    uint32         blockno;      // Numéro de bloc sur le disque
    struct sleeplock lock;       // Verrou d'accès à ce buffer
    uint32         refcnt;       // Nombre de processus utilisant ce buffer
    struct buf    *prev;         // Liste doublement chaînée LRU
    struct buf    *next;
    uint8          data[BSIZE];  // Données du bloc (512 octets)
};

// Cache global de buffers
struct bcache {
    struct spinlock lock;
    struct buf      bufs[NBUF];
    struct buf      head;  // Tête de la liste LRU (doublement chaînée)
};

// Initialiser le cache de buffers
void buf_cache_init(void);

// Obtenir un buffer pour (dev, blockno) — l'alloue si absent du cache
struct buf *buf_get(uint32 dev, uint32 blockno);

// Relâcher un buffer après utilisation
void buf_release(struct buf *b);

void buf_pin(struct buf *b);
void buf_unpin(struct buf *b);

#endif
