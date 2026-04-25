
#ifndef KERNEL_MEM_PAGING_H
#define KERNEL_MEM_PAGING_H

#include "kernel/types.h"
#include "kernel/mem/virtual_memory.h"

// Initialisation
void paging_init(void);

// Création et gestion des tables
void* create_page_table(void);
void free_page_table(void *pagetable);

// Mappages
int map_page(void *pagetable, uint64 virt_addr, uint64 phys_addr, uint64 flags);
void unmap_page(void *pagetable, uint64 virt_addr);

// Traduction d'adresse
uint64 walk_page_table(void *pagetable, uint64 virt_addr);

// Changement de contexte
void switch_page_table(void *pagetable);
void* get_current_page_table(void);

#endif
