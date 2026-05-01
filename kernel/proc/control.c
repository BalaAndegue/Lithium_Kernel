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
int fork(void)
{
    // Récupérer le processus parent (courant)
    struct proc *parent = current_process;
    if (parent == NULL) return -1;
    
    // Allouer une nouvelle structure de processus pour l'enfant
    struct proc *child = alloc_proc();
    if (child == NULL) return -1;
    
    // Relier l'enfant au parent
    child->parent = parent;
    child->sz = parent->sz;  // Même taille de mémoire
    
    // Allouer une pile pour le kernel de l'enfant
    uint64 kstack_page = physmem_alloc_page();
    if (kstack_page == 0) {
        free_proc(child);
        return -1;
    }
    child->kstack = (uint64)phys_to_virt(kstack_page) + PAGE_SIZE;
    
    // Allouer un trapframe pour l'enfant
    uint64 trapframe_page = physmem_alloc_page();
    if (trapframe_page == 0) {
        free_proc(child);
        return -1;
    }
    child->trapframe = (struct trapframe*)phys_to_virt(trapframe_page);
    
    // Copier le trapframe du parent (état des registres)
    if (parent->trapframe != NULL) {
        *child->trapframe = *parent->trapframe;
        child->trapframe->a0 = 0;  // Retour = 0 pour l'enfant
    }
    
    // Créer une nouvelle table des pages pour l'enfant
    child->pagetable = create_page_table();
    if (child->pagetable == NULL) {
        free_proc(child);
        return -1;
    }
    
    // L'enfant est prêt à s'exécuter
    child->state = PROC_RUNNABLE;
    
    printf("fork: created child %d from parent %d\n", child->pid, parent->pid);
    
    // Retourner le PID de l'enfant au parent
    return child->pid;
}

// Placeholder pour les autres fonctions
void exit(int status) {}
int wait(uint64 status_addr) { return -1; }
void sleep(void *chan) {}
void wakeup(void *chan) {}
