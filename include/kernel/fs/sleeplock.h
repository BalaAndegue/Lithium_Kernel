// ===========================================================================
// sleeplock.h - Verrou basé sur le sommeil des processus
// ===========================================================================
// Contrairement au spinlock (attente active), le sleeplock endort le processus
// qui attend. Utilisé pour protéger des ressources dont l'accès est long
// (ex: lecture/écriture disque).
// ===========================================================================

#ifndef KERNEL_FS_SLEEPLOCK_H
#define KERNEL_FS_SLEEPLOCK_H

#include "kernel/types.h"
#include "kernel/mem/spinlock.h"

struct sleeplock {
    int            locked;    // 1 = verrouillé
    struct spinlock spinlk;   // Protège les champs de cette structure
    char          *name;      // Nom (débogage)
    int            pid;       // PID du processus détenteur
};

void sleeplock_init(struct sleeplock *lk, char *name);
void sleeplock_acquire(struct sleeplock *lk);
void sleeplock_release(struct sleeplock *lk);
int  sleeplock_held(struct sleeplock *lk);

#endif
