// ===========================================================================
// process.c - Gestion basique des processus (création, allocation, libération)
// ===========================================================================

#include <stddef.h>
#include "kernel/proc/process.h"
#include "kernel/proc/control.h"
#include "kernel/proc/globals.h"
#include "kernel/mem/physmem.h"
#include "kernel/mem/paging.h"
#include "kernel/mem/layout.h"
#include "kernel/io/console.h"

// Tableau global de tous les processus
struct proc proc_table[NPROC];

// Processus en cours d'exécution (NULL au démarrage)
struct proc *current_process = NULL;

// Prochain PID unique à assigner
int next_pid = 1;

// Retourner le processus courant
struct proc* myproc(void)
{
    return current_process;
}

// Allouer une nouvelle structure de processus
// Cherche une case libre dans la table et l'initialise
struct proc* alloc_proc(void)
{
    // Chercher une case libre
    for (int i = 0; i < NPROC; i++) {
        if (proc_table[i].state == PROC_UNUSED) {
            struct proc *p = &proc_table[i];
            
            // Marquer comme utilisée avec un nouveau PID
            p->state = PROC_USED;
            p->pid = next_pid++;
            
            // Initialiser les autres champs
            p->parent = NULL;
            p->pagetable = NULL;
            p->sz = 0;
            p->kstack = 0;
            p->trapframe = NULL;
            p->exit_code = 0;
            p->name[0] = '\0';
            return p;
        }
    }
    printf("alloc_proc: no more processes!\n");
    return NULL;
}

// Libérer tous les ressources d'un processus
// (mémoire, table des pages, pile...)
void free_proc(struct proc *p)
{
    if (p == NULL) return;
    
    // Libérer la table des pages
    if (p->pagetable != NULL) {
        free_page_table(p->pagetable);
        p->pagetable = NULL;
    }
    
    // Libérer la pile du kernel
    if (p->kstack != 0) {
        uint64 kstack_page = p->kstack - PAGE_SIZE;
        free_physical_page(virt_to_phys((void*)kstack_page));
        p->kstack = 0;
    }
    
    // Libérer le trapframe
    if (p->trapframe != NULL) {
        free_physical_page(virt_to_phys(p->trapframe));
        p->trapframe = NULL;
    }
    
    // Marquer le processus comme non utilisé
    p->state = PROC_UNUSED;
}

// Initialiser la gestion des processus au démarrage
void proc_init(void)
{
    printf("proc_init: initializing process subsystem\n");
    
    // Tout d'abord, marquer tous les processus comme non utilisés
    for (int i = 0; i < NPROC; i++) {
        proc_table[i].state = PROC_UNUSED;
    }
    
    // Créer le processus init (premier processus)
    struct proc *init = alloc_proc();
    if (init == NULL) {
        printf("proc_init: failed!\n");
        return;
    }
    
    // Allouer une pile pour le kernel
    uint64 kstack_page = physmem_alloc_page();
    if (kstack_page == 0) {
        printf("proc_init: failed to allocate kernel stack\n");
        return;
    }
    init->kstack = (uint64)phys_to_virt(kstack_page) + PAGE_SIZE;
    
    // Allouer un trapframe (pour les interruptions)
    uint64 trapframe_page = physmem_alloc_page();
    if (trapframe_page == 0) {
        printf("proc_init: failed to allocate trapframe\n");
        return;
    }
    init->trapframe = (struct trapframe*)phys_to_virt(trapframe_page);
    
    // Créer la table des pages
    init->pagetable = create_page_table();
    if (init->pagetable == NULL) {
        printf("proc_init: failed to create page table\n");
        return;
    }
    
    // Le processus est prêt à s'exécuter
    init->state = PROC_RUNNABLE;
    
    // Donner un nom au processus
    init->name[0] = 'i';
    init->name[1] = 'n';
    init->name[2] = 'i';
    init->name[3] = 't';
    init->name[4] = '\0';
    
    // C'est le processus courant
    current_process = init;
    
    printf("proc_init: init process PID=%d\n", init->pid);
}
