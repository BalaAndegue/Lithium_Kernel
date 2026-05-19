// ============================================================================
// physmem.c - Allocateur de pages physiques
// ============================================================================

#include "kernel/mem/physmem.h"
#include "kernel/mem/layout.h"
#include "kernel/io/console.h"
#include "kernel/types.h"

static uint8 phys_bitmap[NUM_PAGES / 8];
static uint64 free_pages_count = 0;

static int is_page_free(uint64 page_index)
{
    uint64 byte_index = page_index / 8;
    uint64 bit_index = page_index % 8;
    return (phys_bitmap[byte_index] >> bit_index) & 1;
}

static void mark_page_free(uint64 page_index)
{
    uint64 byte_index = page_index / 8;
    uint64 bit_index = page_index % 8;
    phys_bitmap[byte_index] |= (1 << bit_index);
    free_pages_count++;
}

static void mark_page_allocated(uint64 page_index)
{
    uint64 byte_index = page_index / 8;
    uint64 bit_index = page_index % 8;
    phys_bitmap[byte_index] &= ~(1 << bit_index);
    free_pages_count--;
}

void physmem_init(void)
{
    extern uint8 _end;
    uint64 kernel_end = (uint64)&_end;
    uint64 kernel_size;
    uint64 first_free_page;
    uint64 i;
    
    // Afficher les constantes pour debug
    printf("=== PHYS MEMORY DEBUG ===\n");
    printf("sizeof(uint64) = %d\n", sizeof(uint64));
    printf("PAGE_SIZE = %d\n", PAGE_SIZE);
    printf("PHYS_MEM_SIZE = %d\n", PHYS_MEM_SIZE);
    printf("NUM_PAGES = %d\n", NUM_PAGES);
    printf("KERNEL_BASE_ADDR = 0x%lx\n", KERNEL_BASE_ADDR);
    printf("&_end = 0x%lx\n", (uint64)&_end);
    printf("========================\n");
    
    // Vérifier que NUM_PAGES n'est pas 0
    if (NUM_PAGES == 0) {
        printf("FATAL: NUM_PAGES is 0! Check layout.h\n");
        while(1);
    }
    
    kernel_size = kernel_end - KERNEL_BASE_ADDR;
    printf("kernel_size = %d bytes\n", kernel_size);
    
    // Initialiser toutes les pages comme libres
    for (i = 0; i < NUM_PAGES / 8; i++) {
        phys_bitmap[i] = 0xFF;
    }
    free_pages_count = NUM_PAGES;
    
    // Calculer la première page libre après le noyau
    first_free_page = (kernel_size + PAGE_SIZE - 1) / PAGE_SIZE;
    printf("first_free_page = %d\n", first_free_page);
    
    // Vérifier les limites
    if (first_free_page > NUM_PAGES) {
        printf("WARNING: first_free_page (%d) > NUM_PAGES (%d)\n", first_free_page, NUM_PAGES);
        first_free_page = NUM_PAGES;
    }
    
    // Marquer les pages du noyau comme allouées
    for (i = 0; i < first_free_page && i < NUM_PAGES; i++) {
        if (is_page_free(i)) {
            mark_page_allocated(i);
        }
    }
    
    printf("physmem_init: %d pages total, %d pages free\n", NUM_PAGES, free_pages_count);
}

uint64 physmem_alloc_page(void)
{
    uint64 i;
    
    for (i = 0; i < NUM_PAGES; i++) {
        if (is_page_free(i)) {
            mark_page_allocated(i);
            return KERNEL_BASE_ADDR + (i * PAGE_SIZE);
        }
    }
    
    printf("physmem_alloc_page: out of memory!\n");
    return 0;
}

void free_physical_page(uint64 phys_addr)
{
    uint64 page_index;
    
    if (phys_addr < KERNEL_BASE_ADDR || phys_addr >= KERNEL_BASE_ADDR + PHYS_MEM_SIZE) {
        printf("free_physical_page: address 0x%lx out of range\n", phys_addr);
        return;
    }
    
    page_index = (phys_addr - KERNEL_BASE_ADDR) / PAGE_SIZE;
    
    if (page_index >= NUM_PAGES) {
        printf("free_physical_page: page index %d out of range\n", page_index);
        return;
    }
    
    if (!is_page_free(page_index)) {
        mark_page_free(page_index);
    }
}

uint64 get_free_pages_count(void)
{
    return free_pages_count;
}
