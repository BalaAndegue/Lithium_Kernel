// ===========================================================================
// init.c - Premier processus utilisateur du noyau Lithium
// ===========================================================================
//
// init est le premier programme qui s'exécute en espace utilisateur.
// Il est lancé directement par le kernel après l'initialisation.
//
// Rôle d'init :
//   1. Afficher un message de démarrage
//   2. Forker des processus enfants pour les tâches de base
//   3. Rester vivant en boucle pour adopter les processus orphelins
//      (tout processus dont le parent meurt est adopté par init)
// ===========================================================================

#include "kernel/types.h"
#include "user/ulibc.h"
#include "user/syscall.h"

// ---------------------------------------------------------------------------
// Premier processus : affiché au boot, puis boucle sur wait()
// ---------------------------------------------------------------------------
void _start(void)
{
    u_printf("init: Lithium Kernel — premier processus utilisateur\n");
    u_printf("init: PID = %d\n", u_getpid());

    // Forker un shell ou un programme de test quand exec() sera disponible
    int pid = u_fork();

    if (pid == 0) {
        // Processus fils — simuler une tâche courte
        u_printf("init: fils PID=%d démarré\n", u_getpid());
        u_sleep(5);
        u_printf("init: fils PID=%d terminé\n", u_getpid());
        u_exit(0);

    } else if (pid > 0) {
        // Processus parent — attendre les fils et les adopter
        while (1) {
            int child = u_wait();
            if (child > 0) {
                u_printf("init: fils PID=%d recueilli\n", child);
            } else {
                // Plus de fils, ceder la main
                u_yield();
            }
        }

    } else {
        u_printf("init: fork() a échoué\n");
        u_exit(1);
    }
}
