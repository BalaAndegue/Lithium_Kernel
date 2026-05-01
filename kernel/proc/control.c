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
    struct proc *parent = current_process;
    if (parent == NULL) return -1;
    
    struct proc *child = alloc_proc();
    if (child == NULL) return -1;
    
    child->parent = parent;
    child->sz = parent->sz;
    
    uint64 kstack_page = physmem_alloc_page();
    if (kstack_page == 0) {
        free_proc(child);
        return -1;
    }
    child->kstack = (uint64)phys_to_virt(kstack_page) + PAGE_SIZE;
    
    uint64 trapframe_page = physmem_alloc_page();
    if (trapframe_page == 0) {
        free_proc(child);
        return -1;
    }
    child->trapframe = (struct trapframe*)phys_to_virt(trapframe_page);
    
    if (parent->trapframe != NULL) {
        *child->trapframe = *parent->trapframe;
        child->trapframe->a0 = 0;
    }
    
    child->pagetable = create_page_table();
    if (child->pagetable == NULL) {
        free_proc(child);
        return -1;
    }
    
    child->state = PROC_RUNNABLE;
    
    printf("fork: created child %d from parent %d\n", child->pid, parent->pid);
    
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
    // Chercher un enfant qui est un zombie
    for (int i = 0; i < NPROC; i++) {
        struct proc *child = &proc_table[i];
        if (child->parent == current_process && child->state == PROC_ZOMBIE) {
            // Enfant trouvé!
            int pid = child->pid;
            
            // Copier le code de sortie à l'adresse donnée
            *(int*)status_addr = child->exit_code;
            
            // Libérer les ressources de l'enfant
            free_proc(child);
            
            return pid;
        }
    }
    return -1;  // Pas d'enfant trouvé
}

void sleep(void *chan) {}
void wakeup(void *chan) {}
