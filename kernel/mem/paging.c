
// Page table management

#include <stddef.h>
#include "kernel/types.h"
#include "kernel/mem/layout.h"
#include "kernel/mem/physmem.h"
#include "kernel/mem/virtual_memory.h"
#include "kernel/io/console.h"

static void *kernel_pagetable = NULL;

// Create a new page table
void* create_page_table(void)
{
    // Allocate physical page
    uint64 phys = physmem_alloc_page();
    if (phys == 0) return NULL;
    
    uint64 *table = (uint64*)phys_to_virt(phys);
    for (int i = 0; i < 512; i++) {
        table[i] = 0;
    }
    return table;
}

// Free a page table
void free_page_table(void *pagetable)
{
    if (pagetable != NULL) {
        free_physical_page(virt_to_phys(pagetable));
    }
}

// Walk the page tables to find the PTE
static uint64* walk(void *pagetable, uint64 virt_addr, int alloc)
{
    uint64 vpn2 = VPN2(virt_addr);
    uint64 vpn1 = VPN1(virt_addr);
    uint64 vpn0 = VPN0(virt_addr);
    
    // Level 2
    uint64 *pte_l2 = &((uint64*)pagetable)[vpn2];
    
    if (!(*pte_l2 & PTE_V)) {
        if (!alloc) return NULL;
        // Allocate page for next level
        uint64 phys = physmem_alloc_page();
        if (phys == 0) return NULL;
        *pte_l2 = MAKE_PTE(phys, PTE_V);
        uint64 *level1 = (uint64*)phys_to_virt(phys);
        for (int i = 0; i < 512; i++) level1[i] = 0;
    }
    
    // Level 1
    uint64 *level1 = (uint64*)phys_to_virt(PTE_ADDR(*pte_l2));
    uint64 *pte_l1 = &level1[vpn1];
    
    if (!(*pte_l1 & PTE_V)) {
        if (!alloc) return NULL;
        // Allocate page for next level
        uint64 phys = physmem_alloc_page();
        if (phys == 0) return NULL;
        *pte_l1 = MAKE_PTE(phys, PTE_V);
        uint64 *level0 = (uint64*)phys_to_virt(phys);
        for (int i = 0; i < 512; i++) level0[i] = 0;
    }
    
    // Level 0
    uint64 *level0 = (uint64*)phys_to_virt(PTE_ADDR(*pte_l1));
    return &level0[vpn0];
}

// Map a virtual page to a physical page
int map_page(void *pagetable, uint64 virt_addr, uint64 phys_addr, uint64 flags)
{
    uint64 *pte = walk(pagetable, virt_addr, 1);
    if (pte == NULL) return -1;
    if (*pte & PTE_V) return -1;
    *pte = MAKE_PTE(phys_addr, flags | PTE_V);
    return 0;
}

// Unmap a virtual page
void unmap_page(void *pagetable, uint64 virt_addr)
{
    uint64 *pte = walk(pagetable, virt_addr, 0);
    if (pte != NULL && (*pte & PTE_V)) {
        *pte = 0;
    }
}

// Translate virtual address to physical address
uint64 walk_page_table(void *pagetable, uint64 virt_addr)
{
    uint64 *pte = walk(pagetable, virt_addr, 0);
    if (pte != NULL && (*pte & PTE_V)) {
        return PTE_ADDR(*pte) + (virt_addr & (PAGE_SIZE - 1));
    }
    return 0;
}

// Switch to the given page table
void switch_page_table(void *pagetable)
{
    uint64 phys = virt_to_phys(pagetable);
    uint64 satp = (8UL << 60) | ((phys >> 12) & 0x0FFFFFFFFFFFFFUL);
    
    asm volatile("csrw satp, %0" : : "r"(satp));
    asm volatile("sfence.vma zero, zero");
}

// Get the current active page table
void* get_current_page_table(void)
{
    return kernel_pagetable;
}

// Initialize virtual memory paging
void paging_init(void)
{
    uint64 i;
    
    printf("paging_init: initializing SV39 page tables\n");
    
    kernel_pagetable = create_page_table();
    if (kernel_pagetable == NULL) {
        printf("paging_init: failed to create kernel page table\n");
        return;
    }
    
    // Identity map the first 128 MB
    for (i = 0; i < PHYS_MEM_SIZE; i += PAGE_SIZE) {
        if (map_page(kernel_pagetable, i, i, PTE_R | PTE_W | PTE_X) != 0) {
            printf("paging_init: failed to map page %p\n", i);
            return;
        }
    }
    
    switch_page_table(kernel_pagetable);
    printf("paging_init: kernel page table activated\n");
}
