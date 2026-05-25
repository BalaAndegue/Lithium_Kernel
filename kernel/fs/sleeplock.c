// ===========================================================================
// sleeplock.c - Verrou avec mise en sommeil du processus
// ===========================================================================

#include <stddef.h>
#include "kernel/types.h"
#include "kernel/fs/sleeplock.h"
#include "kernel/proc/control.h"
#include "kernel/proc/globals.h"
#include "kernel/io/console.h"

void sleeplock_init(struct sleeplock *lk, char *name)
{
    lk->locked = 0;
    lk->name   = name;
    lk->pid    = 0;
    spinlock_init(&lk->spinlk, "sleeplock");
}

// Acquérir le sleeplock : si déjà pris, endormir jusqu'à sa libération
void sleeplock_acquire(struct sleeplock *lk)
{
    spinlock_acquire(&lk->spinlk);

    // Tant que le verrou est pris par quelqu'un d'autre, dormir
    while (lk->locked) {
        // sleep() relâche le spinlock, dort, et le réacquiert au réveil
        sleep(lk, &lk->spinlk);
    }

    lk->locked = 1;
    if (current_process != NULL)
        lk->pid = current_process->pid;

    spinlock_release(&lk->spinlk);
}

// Relâcher le sleeplock et réveiller les processus en attente
void sleeplock_release(struct sleeplock *lk)
{
    spinlock_acquire(&lk->spinlk);

    lk->locked = 0;
    lk->pid    = 0;

    // Réveiller tous les processus qui dormaient sur ce verrou
    wakeup(lk);

    spinlock_release(&lk->spinlk);
}

int sleeplock_held(struct sleeplock *lk)
{
    int r;
    spinlock_acquire(&lk->spinlk);
    r = lk->locked && (current_process != NULL) && (lk->pid == current_process->pid);
    spinlock_release(&lk->spinlk);
    return r;
}
