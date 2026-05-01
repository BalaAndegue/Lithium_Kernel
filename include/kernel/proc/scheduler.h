// ===========================================================================
// scheduler.h - Ordonnanceur (choisit quel processus exécuter)
// ===========================================================================

#ifndef KERNEL_PROC_SCHEDULER_H
#define KERNEL_PROC_SCHEDULER_H

#include "kernel/proc/process.h"

// Exécuter l'ordonnanceur (choisir le prochain processus)
void scheduler(void);

// Donner la main à un autre processus (yield)
void yield(void);

// Changement de contexte en assembleur (défini dans switch_context.S)
// Sauvegarde les registres du processus actuel et restaure ceux du nouveau
void context_switch(struct context *old, struct context *new);

#endif  