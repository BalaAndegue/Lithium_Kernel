// ===========================================================================
// inode.h - Structure et interface des inodes
// ===========================================================================
// Un inode = les métadonnées d'un fichier (type, taille, blocs de données).
// Il y a deux représentations :
//   - dinode : l'inode sur le disque (compact, BSIZE/sizeof(dinode) par bloc)
//   - inode  : l'inode en mémoire (avec verrou, compteur de référence)
// ===========================================================================

#ifndef KERNEL_FS_INODE_H
#define KERNEL_FS_INODE_H

#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/fs/sleeplock.h"
#include "kernel/fs/file_stat.h"

// ---------------------------------------------------------------------------
// dinode - Inode tel qu'il est stocké sur le disque
// ---------------------------------------------------------------------------
struct dinode {
    short  type;              // T_DIR, T_FILE, T_DEV (0 = libre)
    short  major;             // Numéro majeur (pour T_DEV)
    short  minor;             // Numéro mineur (pour T_DEV)
    short  nlink;             // Nombre de répertoires qui référencent cet inode
    uint32 size;              // Taille du fichier en octets
    uint32 addrs[NDIRECT+1];  // Blocs de données (NDIRECT directs + 1 indirect)
};

// Nombre d'inodes par bloc disque
#define IPB (BSIZE / sizeof(struct dinode))

// Numéro de bloc contenant l'inode numéro 'inum'
#define IBLOCK(inum, sb) ((inum) / IPB + (sb).inodestart)

// ---------------------------------------------------------------------------
// inode - Inode en mémoire (cache)
// ---------------------------------------------------------------------------
struct inode {
    uint32         dev;       // Numéro de périphérique
    uint32         inum;      // Numéro d'inode
    int            ref;       // Compteur de références
    struct sleeplock lock;    // Verrou d'accès à cet inode
    int            valid;     // Les champs ci-dessous ont été lus du disque ?

    // Copie des champs du dinode
    short          type;
    short          major;
    short          minor;
    short          nlink;
    uint32         size;
    uint32         addrs[NDIRECT+1];
};

// Superbloc — premier bloc du disque, décrit la structure du FS
struct superblock {
    uint32 magic;         // Nombre magique pour identifier le FS
    uint32 size;          // Taille totale du FS en blocs
    uint32 nblocks;       // Nombre de blocs de données
    uint32 ninodes;       // Nombre d'inodes
    uint32 nlog;          // Nombre de blocs de log
    uint32 logstart;      // Numéro du premier bloc de log
    uint32 inodestart;    // Numéro du premier bloc d'inodes
    uint32 bmapstart;     // Numéro du premier bloc de bitmap
};

#define FSMAGIC 0x10203040

// Interface
void            inode_init(int dev);
struct inode   *inode_alloc(uint32 dev, short type);
struct inode   *inode_get(uint32 dev, uint32 inum);
void            inode_lock(struct inode *ip);
void            inode_unlock(struct inode *ip);
void            inode_put(struct inode *ip);
void            inode_unlock_put(struct inode *ip);
void            inode_update(struct inode *ip);
int             inode_read(struct inode *ip, int user_dst, uint64 dst, uint32 off, uint32 n);
int             inode_write(struct inode *ip, int user_src, uint64 src, uint32 off, uint32 n);
void            inode_stat(struct inode *ip, struct stat *st);
struct inode   *inode_dup(struct inode *ip);

#endif
