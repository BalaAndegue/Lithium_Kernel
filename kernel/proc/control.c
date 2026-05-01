// ===========================================================================
// control.c - fork(), exit(), wait(), sleep(), wakeup()
// ===========================================================================

#include <stddef.h>
#include "kernel/proc/process.h"
#include "kernel/proc/control.h"
#include "kernel/proc/scheduler.h"
#include "kernel/proc/globals.h"
#include "kernel/mem/physmem.h"
#include "kernel/mem/paging.h"
#include "kernel/mem/layout.h"
#include "kernel/io/console.h"

// Créer un processus enfant (copie du processus courant)
// C'est la fonction fork() classique d'UNIX
int fork(void)
{
    // Récupérer le processus parent (celui qui appelle fork)
    struct proc *parent = current_process;
    if (parent == NULL) return -1;
    
    // Allouer une nouvelle structure pour l'enfant
    struct proc *child = alloc_proc();
    if (child == NULL) return -1;
    
    // Lier l'enfant à son parent
    child->parent = parent;
    child->sz = parent->sz;  // Même taille de mémoire que le parent
    
    // Allouer une pile pour le kernel de l'enfant
    uint64 kstack_page = physmem_alloc_page();
    if (kstack_page == 0) {
        free_proc(child);
        return -1;
    }
    child->kstack = (uint64)phys_to_virt(kstack_page) + PAGE_SIZE;
    
    // Allouer un trapframe pour sauvegarder les registres
    uint64 trapframe_page = physmem_alloc_page();
    if (trapframe_page == 0) {
        free_proc(child);
        return -1;
    }
    child->trapframe = (struct trapframe*)phys_to_virt(trapframe_page);
    
    // Copier le trapframe du parent (état des registres au moment du fork)
    if (parent->trapframe != NULL) {
        *child->trapframe = *parent->trapframe;
        child->trapframe->a0 = 0;  // Pour l'enfant, fork() retourne 0
    }
    
    // Créer une nouvelle table des pages pour l'enfant
    child->pagetable = create_page_table();
    if (child->pagetable == NULL) {
        free_proc(child);
        return -1;
    }
    
    // L'enfant est maintenant prêt à tourner
    child->state = PROC_RUNNABLE;
    
    printf("fork: processus enfant %d créé depuis parent %d\n", child->pid, parent->pid);
    
    // Retourner le PID de l'enfant au parent
    return child->pid;
}

// Terminer le processus courant
void exit(int status)
{
    struct proc *p = current_process;
    if (p == NULL) return;
    
    printf("exit: process %d exited with status %d\n", p->pid, status);
    
    p->exit_code = status;
    p->state = PROC_ZOMBIE;
    
    if (p->parent != NULL) {
        wakeup(p->parent);
    }
    
    while (1) {
        asm volatile("wfi");
    }
}

// Attendre qu'un enfant se termine et récupérer son code de sortie
int wait(uint64 status_addr)
{
    for (int i = 0; i < NPROC; i++) {
        struct proc *child = &proc_table[i];
        if (child->parent == current_process && child->state == PROC_ZOMBIE) {
            int pid = child->pid;
            *(int*)status_addr = child->exit_code;
            free_proc(child);
            return pid;
        }
    }
    return -1;
}

// Mettre le processus courant en sleep (en attente)
void sleep(void *chan)
{
    struct proc *p = current_process;
    if (p == NULL) return;
    
    (void)chan;  // Paramètre non utilisé pour le moment
    
    // Marquer le processus comme dormant
    p->state = PROC_SLEEPING;
    
    // Appeler l'ordonnanceur pour choisir un autre processus
    scheduler();
}

// Réveiller tous les processus qui dorment
void wakeup(void *chan)
{
    (void)chan;  // Paramètre non utilisé pour le moment
    
    // Parcourir tous les processus
    for (int i = 0; i < NPROC; i++) {
        // Si c'est un qui dort, le rendre exécutable
        if (proc_table[i].state == PROC_SLEEPING) {
            proc_table[i].state = PROC_RUNNABLE;
        }
    }
}
