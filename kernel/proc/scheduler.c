// ===========================================================================
// scheduler.c - Ordonnanceur round-robin (choisit quel processus exécuter)
// ===========================================================================

#include <stddef.h>
#include "kernel/proc/process.h"
#include "kernel/proc/control.h"
#include "kernel/proc/scheduler.h"
#include "kernel/proc/globals.h"
#include "kernel/mem/paging.h"
#include "kernel/io/console.h"

// Libérer le processeur: le processus courant arrête et on choisit un autre
void yield(void)
{
    // Mettre le processus courant comme exécutable (non comme running)
    if (current_process != NULL && current_process->state == PROC_RUNNING) {
        current_process->state = PROC_RUNNABLE;
    }
    
    // Appeler l'ordonnanceur pour choisir le prochain processus
    scheduler();
}

// Ordonnanceur: cherche un processus exécutable et le lance
// Fait tourner les processus à tour de rôle (round-robin)
void scheduler(void)
{
    // Index du processus courant (pour le round-robin)
    static int current_index = 0;
    
    // Boucle infinie: parcourir les processus et les exécuter
    while (1) {
        // Vérifier tous NPROC processus
        for (int i = 0; i < NPROC; i++) {
            struct proc *p = &proc_table[current_index];
            current_index = (current_index + 1) % NPROC;
            
            // Trouver un processus exécutable
            if (p->state == PROC_RUNNABLE) {
                struct proc *prev = current_process;
                
                // Mettre à jour le processus courant
                current_process = p;
                p->state = PROC_RUNNING;
                
                // Charger sa table des pages
                if (p->pagetable != NULL) {
                    switch_page_table(p->pagetable);
                }
                
                // Sauvegarder l'ancien contexte et restaurer le nouveau
                if (prev != NULL) {
                    context_switch(&prev->context, &p->context);
                } else {
                    context_switch(NULL, &p->context);
                }
            }
        }
    }
}
