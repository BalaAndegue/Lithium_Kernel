// ===========================================================================
// spinlock.h - Verrou d'exclusion mutuelle par attente active (spin)
// ===========================================================================
// Un spinlock protège une ressource partagée en désactivant les interruptions
// et en tournant en boucle jusqu'à pouvoir acquérir le verrou.
// À utiliser pour des sections critiques TRÈS courtes (quelques instructions).
// ===========================================================================

#ifndef KERNEL_MEM_SPINLOCK_H
#define KERNEL_MEM_SPINLOCK_H

#include "kernel/types.h"

struct spinlock {
    int locked;       // 1 = verrouillé, 0 = libre
    char *name;       // Nom du verrou (pour le débogage)
};

void spinlock_init(struct spinlock *lk, char *name);
void spinlock_acquire(struct spinlock *lk);
void spinlock_release(struct spinlock *lk);
int  spinlock_held(struct spinlock *lk);

#endif
