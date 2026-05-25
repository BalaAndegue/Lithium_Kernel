// ===========================================================================
// file_stat.h - Informations sur un fichier (résultat de stat())
// ===========================================================================

#ifndef KERNEL_FS_FILE_STAT_H
#define KERNEL_FS_FILE_STAT_H

#include "kernel/types.h"

#define T_DIR   1   // Répertoire
#define T_FILE  2   // Fichier ordinaire
#define T_DEV   3   // Périphérique

struct stat {
    int    dev;     // Numéro de périphérique
    uint32 ino;     // Numéro d'inode
    short  type;    // T_DIR, T_FILE ou T_DEV
    short  nlink;   // Nombre de liens vers cet inode
    uint64 size;    // Taille en octets
};

#endif
