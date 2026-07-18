// ===========================================================================
// log.h - Journal d'écriture anticipée (write-ahead log)
// ===========================================================================
// Le log garantit qu'une transaction est soit complète, soit absente du disque.
// Toute écriture sur le système de fichiers passe par le log.
// ===========================================================================

#ifndef KERNEL_FS_LOG_H
#define KERNEL_FS_LOG_H

#include "kernel/types.h"
#include "kernel/fs/buffer.h"

// Initialiser le log (lecture de l'en-tête log sur le disque)
void log_init(int dev, int start, int size);

// Début d'une transaction — peut bloquer si le log est plein
void log_begin_op(void);

// Fin d'une transaction — déclenche l'écriture si toutes les ops sont finies
void log_end_op(void);

// Écrire un buffer dans le log (à appeler entre begin_op et end_op)
void log_write(struct buf *b);

#endif
