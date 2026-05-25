// ===========================================================================
// block.h - Interface couche bloc (lecture/écriture sur le disque virtuel)
// ===========================================================================

#ifndef KERNEL_FS_BLOCK_H
#define KERNEL_FS_BLOCK_H

#include "kernel/types.h"
#include "kernel/fs/buffer.h"

// Lire le bloc numéro 'blockno' depuis le disque dans un buffer cache
struct buf *block_read(uint32 dev, uint32 blockno);

// Écrire le contenu d'un buffer modifié sur le disque
void block_write(struct buf *b);

// Libérer un buffer (décrémenter son compteur de référence)
void block_release(struct buf *b);

// Épingler/dépingler un buffer (empêcher l'éviction du cache)
void block_pin(struct buf *b);
void block_unpin(struct buf *b);

#endif
