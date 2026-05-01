// ===========================================================================
// globals.h - Variables globales partagées entre tous les fichiers proc
// ===========================================================================

#ifndef KERNEL_PROC_GLOBALS_H
#define KERNEL_PROC_GLOBALS_H

#include "kernel/proc/process.h"

// Nombre maximum de processus
#define NPROC 64

// Tableau de tous les processus
extern struct proc proc_table[NPROC];

// Processus en cours d'exécution
extern struct proc *current_process;

// Prochain PID à assigner (numéro de processus unique)
extern int next_pid;

#endif
