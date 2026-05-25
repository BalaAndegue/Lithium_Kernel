// ===========================================================================
// file.h - Abstraction fichier (inode, pipe ou périphérique)
// ===========================================================================

#ifndef KERNEL_FS_FILE_H
#define KERNEL_FS_FILE_H

#include "kernel/types.h"
#include "kernel/mem/spinlock.h"
#include "kernel/param.h"

// Types de fichier
#define FD_NONE   0
#define FD_PIPE   1
#define FD_INODE  2
#define FD_DEVICE 3

struct pipe;
struct inode;

struct file {
    int            type;    // FD_NONE, FD_PIPE, FD_INODE, FD_DEVICE
    int            ref;     // Compteur de références
    char           readable;
    char           writable;
    struct pipe   *pipe;    // Valide si type == FD_PIPE
    struct inode  *ip;      // Valide si type == FD_INODE ou FD_DEVICE
    uint32         off;     // Position courante (pour FD_INODE)
    short          major;   // Numéro majeur (pour FD_DEVICE)
};

// Table globale des fichiers ouverts
struct ftable {
    struct spinlock lock;
    struct file     file[NFILE];
};

void         file_init(void);
struct file *file_alloc(void);
struct file *file_dup(struct file *f);
void         file_close(struct file *f);
int          file_read(struct file *f, uint64 addr, int n);
int          file_write(struct file *f, uint64 addr, int n);
int          file_stat(struct file *f, uint64 addr);

#endif
