// ===========================================================================
// param.h - Constantes globales du noyau Lithium
// ===========================================================================

#ifndef KERNEL_PARAM_H
#define KERNEL_PARAM_H

#define BSIZE        512    // Taille d'un bloc disque en octets
#define NBUF          30    // Nombre de buffers dans le cache
#define NINODE        50    // Nombre max d'inodes en mémoire
#define NFILE        100    // Nombre max de fichiers ouverts
#define MAXOPBLOCKS   10    // Max blocs par opération de log
#define LOGSIZE      (MAXOPBLOCKS * 3)  // Taille du log en blocs
#define FSSIZE      1000    // Taille totale du système de fichiers en blocs
#define ROOTINO        1    // Numéro d'inode de la racine "/"
#define NDIRECT       12    // Blocs directs par inode
#define NINDIRECT  (BSIZE / sizeof(uint32))  // Blocs indirects
#define MAXFILE    (NDIRECT + NINDIRECT)     // Taille max d'un fichier en blocs

#endif
