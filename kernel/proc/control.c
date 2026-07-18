// ============================================================================
// control.c - fork(), exit(), wait(), sleep(), wakeup()
// ============================================================================

#include <stddef.h>
#include "kernel/proc/process.h"
#include "kernel/proc/control.h"
#include "kernel/proc/scheduler.h"
#include "kernel/proc/globals.h"
#include "kernel/mem/physmem.h"
#include "kernel/mem/paging.h"
#include "kernel/mem/layout.h"
#include "kernel/mem/spinlock.h"
#include "kernel/io/console.h"

// ----------------------------------------------------------------------------
// copy_trapframe - Copie manuelle d'un trapframe (évite memcpy)
// ----------------------------------------------------------------------------
static void copy_trapframe(struct trapframe *dest, struct trapframe *src)
{
    if (dest == NULL || src == NULL) return;
    
    dest->kernel_satp = src->kernel_satp;
    dest->kernel_sp = src->kernel_sp;
    dest->kernel_trap = src->kernel_trap;
    dest->epc = src->epc;
    dest->kernel_hartid = src->kernel_hartid;
    dest->ra = src->ra;
    dest->sp = src->sp;
    dest->gp = src->gp;
    dest->tp = src->tp;
    dest->t0 = src->t0;
    dest->t1 = src->t1;
    dest->t2 = src->t2;
    dest->s0 = src->s0;
    dest->s1 = src->s1;
    dest->a0 = src->a0;
    dest->a1 = src->a1;
    dest->a2 = src->a2;
    dest->a3 = src->a3;
    dest->a4 = src->a4;
    dest->a5 = src->a5;
    dest->a6 = src->a6;
    dest->a7 = src->a7;
    dest->s2 = src->s2;
    dest->s3 = src->s3;
    dest->s4 = src->s4;
    dest->s5 = src->s5;
    dest->s6 = src->s6;
    dest->s7 = src->s7;
    dest->s8 = src->s8;
    dest->s9 = src->s9;
    dest->s10 = src->s10;
    dest->s11 = src->s11;
    dest->t3 = src->t3;
    dest->t4 = src->t4;
    dest->t5 = src->t5;
    dest->t6 = src->t6;
}

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
    
    // Copie manuelle du trapframe au lieu de memcpy
    if (parent->trapframe != NULL) {
        copy_trapframe(child->trapframe, parent->trapframe);
        child->trapframe->a0 = 0;  // fork retourne 0 pour l'enfant
    }
    
    child->pagetable = create_page_table();
    if (child->pagetable == NULL) {
        free_proc(child);
        return -1;
    }
    
    child->state = PROC_RUNNABLE;
    
    printf("fork: processus enfant %d cree depuis parent %d\n", child->pid, parent->pid);
    
    return child->pid;
}

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

void sleep(void *chan, struct spinlock *lk)
{
    struct proc *p = current_process;
    if (p == NULL) return;
    
    (void)chan;
    
    // Release spinlock before sleeping
    if (lk != NULL)
        spinlock_release(lk);
    
    p->state = PROC_SLEEPING;
    scheduler();
    
    // Re-acquire spinlock after waking up
    if (lk != NULL)
        spinlock_acquire(lk);
}

void wakeup(void *chan)
{
    (void)chan;
    
    for (int i = 0; i < NPROC; i++) {
        if (proc_table[i].state == PROC_SLEEPING) {
            proc_table[i].state = PROC_RUNNABLE;
        }
    }
}
