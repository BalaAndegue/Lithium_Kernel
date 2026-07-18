
// ===========================================================================
// control.h - Fonctions pour contrôler les processus: fork, exit, sleep...
// ===========================================================================

#ifndef KERNEL_PROC_CONTROL_H
#define KERNEL_PROC_CONTROL_H

#include "kernel/types.h"
#include "kernel/proc/process.h"
#include "kernel/mem/spinlock.h"

// Initialiser la gestion des processus
void proc_init(void);

// Créer et terminer un processus
int fork(void);           // Crée un processus enfant
void exit(int status);    // Termine le processus courant
int wait(uint64 status_addr);  // Attendre qu'un enfant se termine

// Accéder au processus courant
struct proc* myproc(void);

// Faire dormir et réveiller des processus
void sleep(void *chan, struct spinlock *lk);   // Endormir le processus courant (lk optionnel)
void wakeup(void *chan);  // Réveiller les processus qui dorment

// Allouer et libérer un processus
void free_proc(struct proc *p);     // Libérer les ressources d'un processus
struct proc* alloc_proc(void);      // Créer une nouvelle structure de processus

#endif 
