
// Physical memory page allocator using a bitmap

#include "kernel/mem/physmem.h"
#include "kernel/mem/layout.h"
#include "kernel/io/console.h"
#include "kernel/types.h"

// Bitmap des pages physiques (1 bit par page)
static uint8 phys_bitmap[NUM_PAGES / 8];
static uint64 free_pages_count;

// Check if a page is free
static int is_page_free(uint64 page_index)
{
    uint64 byte_index = page_index / 8;
    uint64 bit_index = page_index % 8;
    return (phys_bitmap[byte_index] >> bit_index) & 1;
}

// Mark a page as free
static void mark_page_free(uint64 page_index)
{
    uint64 byte_index = page_index / 8;
    uint64 bit_index = page_index % 8;
    phys_bitmap[byte_index] |= (1 << bit_index);
    free_pages_count++;
}

// Mark a page as allocated
static void mark_page_allocated(uint64 page_index)
{
    uint64 byte_index = page_index / 8;
    uint64 bit_index = page_index % 8;
    phys_bitmap[byte_index] &= ~(1 << bit_index);
    free_pages_count--;
}

// Initialize the physical memory allocator
void physmem_init(void)
{
    uint64 i;
    extern uint8 _end;
    uint64 kernel_end = (uint64)&_end;
    uint64 first_free_page;
    
    // Mark all pages as free
    for (i = 0; i < NUM_PAGES / 8; i++) {
        phys_bitmap[i] = 0xFF;
    }
    free_pages_count = NUM_PAGES;
    
    // Mark kernel pages as allocated
    first_free_page = (kernel_end + PAGE_SIZE - 1) / PAGE_SIZE;
    for (i = 0; i < first_free_page; i++) {
        if (is_page_free(i)) {
            mark_page_allocated(i);
        }
    }
    
    printf("physmem_init: %d pages total, %d pages free\n", 
           NUM_PAGES, free_pages_count);
}

// Allocate a physical page
uint64 physmem_alloc_page(void)
{
    uint64 i;
    
    for (i = 0; i < NUM_PAGES; i++) {
        if (is_page_free(i)) {
            mark_page_allocated(i);
            return i * PAGE_SIZE;
        }
    }
    
    printf("physmem_alloc_page: out of memory!\n");
    return 0;
}

// Free a physical page
void free_physical_page(uint64 phys_addr)
{
    uint64 page_index = phys_addr / PAGE_SIZE;
    
    if (phys_addr % PAGE_SIZE != 0) {
        printf("free_physical_page: address %p not page-aligned\n", (void*)phys_addr);
        return;
    }
    
    if (page_index >= NUM_PAGES) {
        printf("free_physical_page: address %p out of range\n", (void*)phys_addr);
        return;
    }
    
    if (!is_page_free(page_index)) {
        mark_page_free(page_index);
    }
}

// Get the count of free pages
uint64 get_free_pages_count(void)
{
    return free_pages_count;
}
