// ===========================================================================
// pipe.c - Pipe : canal de communication inter-processus (IPC)
// ===========================================================================
//
// Un pipe est un buffer circulaire partagé entre deux processus.
// Un processus écrit (write end), l'autre lit (read end).
// Si le buffer est plein, pipe_write() dort. Si vide, pipe_read() dort.
// ===========================================================================

#include <stddef.h>
#include "kernel/types.h"
#include "kernel/fs/pipe.h"
#include "kernel/fs/file.h"
#include "kernel/mem/physmem.h"
#include "kernel/mem/layout.h"
#include "kernel/proc/control.h"
#include "kernel/io/console.h"

// ---------------------------------------------------------------------------
// pipe_alloc - Allouer un nouveau pipe et ses deux descripteurs
// ---------------------------------------------------------------------------
int pipe_alloc(struct file **f0, struct file **f1)
{
    struct pipe *pi = NULL;
    *f0 = NULL;
    *f1 = NULL;

    // Allouer la structure pipe dans une page physique
    uint64 page = physmem_alloc_page();
    if (page == 0) goto bad;
    pi = (struct pipe *)phys_to_virt(page);

    // Allouer les deux descripteurs de fichier
    if ((*f0 = file_alloc()) == NULL || (*f1 = file_alloc()) == NULL)
        goto bad;

    spinlock_init(&pi->lock, "pipe");
    pi->readopen  = 1;
    pi->writeopen = 1;
    pi->nread     = 0;
    pi->nwrite    = 0;

    (*f0)->type     = FD_PIPE;
    (*f0)->readable = 1;
    (*f0)->writable = 0;
    (*f0)->pipe     = pi;

    (*f1)->type     = FD_PIPE;
    (*f1)->readable = 0;
    (*f1)->writable = 1;
    (*f1)->pipe     = pi;

    return 0;

bad:
    if (pi)  free_physical_page(virt_to_phys(pi));
    if (*f0) file_close(*f0);
    if (*f1) file_close(*f1);
    return -1;
}

// ---------------------------------------------------------------------------
// pipe_close - Fermer une extrémité du pipe
// ---------------------------------------------------------------------------
void pipe_close(struct pipe *pi, int writable)
{
    spinlock_acquire(&pi->lock);

    if (writable) {
        pi->writeopen = 0;
        wakeup(&pi->nread);  // Réveiller les lecteurs bloqués
    } else {
        pi->readopen = 0;
        wakeup(&pi->nwrite); // Réveiller les écrivains bloqués
    }

    if (pi->readopen == 0 && pi->writeopen == 0) {
        spinlock_release(&pi->lock);
        free_physical_page(virt_to_phys(pi));
    } else {
        spinlock_release(&pi->lock);
    }
}

// ---------------------------------------------------------------------------
// pipe_write - Écrire n octets dans le pipe
// ---------------------------------------------------------------------------
int pipe_write(struct pipe *pi, uint64 addr, int n)
{
    spinlock_acquire(&pi->lock);

    for (int i = 0; i < n; i++) {
        // Attendre si le buffer est plein
        while (pi->nwrite == pi->nread + PIPESIZE) {
            if (!pi->readopen) {
                spinlock_release(&pi->lock);
                return -1;
            }
            wakeup(&pi->nread);
            sleep(&pi->nwrite, &pi->lock);
        }
        pi->data[pi->nwrite++ % PIPESIZE] = ((char *)addr)[i];
    }

    wakeup(&pi->nread);
    spinlock_release(&pi->lock);
    return n;
}

// ---------------------------------------------------------------------------
// pipe_read - Lire n octets depuis le pipe
// ---------------------------------------------------------------------------
int pipe_read(struct pipe *pi, uint64 addr, int n)
{
    spinlock_acquire(&pi->lock);

    // Attendre qu'il y ait quelque chose à lire
    while (pi->nread == pi->nwrite && pi->writeopen) {
        sleep(&pi->nread, &pi->lock);
    }

    int i;
    for (i = 0; i < n && pi->nread < pi->nwrite; i++) {
        ((char *)addr)[i] = pi->data[pi->nread++ % PIPESIZE];
    }

    wakeup(&pi->nwrite);
    spinlock_release(&pi->lock);
    return i;
}
