// ===========================================================================
// spinlock.c - Verrou spinlock avec attente active
// ===========================================================================

#include "kernel/types.h"
#include "kernel/mem/spinlock.h"
#include "kernel/io/console.h"
#include "riscv/intrinsics.h"

void spinlock_init(struct spinlock *lk, char *name)
{
    lk->locked = 0;
    lk->name   = name;
}

// Acquérir le verrou — désactive les interruptions puis tourne jusqu'à l'obtenir
void spinlock_acquire(struct spinlock *lk)
{
    intr_off();

    if (spinlock_held(lk)) {
        printf("spinlock_acquire: double acquire sur '%s'\n", lk->name);
        while (1);
    }

    // __sync_lock_test_and_set : échange atomique, retourne l'ancienne valeur
    // On tourne tant que l'ancienne valeur était 1 (déjà verrouillé)
    while (__sync_lock_test_and_set(&lk->locked, 1) != 0)
        ;

    // Barrière mémoire : les accès suivants ne seront pas réordonnancés avant
    __sync_synchronize();
}

// Relâcher le verrou et réactiver les interruptions
void spinlock_release(struct spinlock *lk)
{
    if (!spinlock_held(lk)) {
        printf("spinlock_release: verrou '%s' non tenu\n", lk->name);
        while (1);
    }

    __sync_synchronize();
    __sync_lock_release(&lk->locked);

    intr_on();
}

int spinlock_held(struct spinlock *lk)
{
    return lk->locked;
}
