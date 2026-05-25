// ===========================================================================
// pipe.h - Pipe : communication inter-processus unidirectionnelle
// ===========================================================================

#ifndef KERNEL_FS_PIPE_H
#define KERNEL_FS_PIPE_H

#include "kernel/types.h"
#include "kernel/mem/spinlock.h"

#define PIPESIZE 512   // Taille du buffer circulaire du pipe

struct pipe {
    struct spinlock lock;
    char            data[PIPESIZE];
    uint32          nread;    // Nombre total d'octets lus
    uint32          nwrite;   // Nombre total d'octets écrits
    int             readopen; // 1 si l'extrémité lecture est ouverte
    int             writeopen;// 1 si l'extrémité écriture est ouverte
};

struct file;

int  pipe_alloc(struct file **f0, struct file **f1);
void pipe_close(struct pipe *pi, int writable);
int  pipe_write(struct pipe *pi, uint64 addr, int n);
int  pipe_read(struct pipe *pi, uint64 addr, int n);

#endif
