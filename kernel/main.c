
#include "kernel/io/uart.h"
#include "kernel/io/console.h"
#include "kernel/mem/physmem.h"
#include "kernel/mem/paging.h"

// Entry point of the Lithium Kernel
void kernel_main(void)
{
    // Initialize UART for serial communication
    uart_init();

    // Print boot messages
    printf("Lithium Kernel starting...\n");
    printf("========================================\n");
    
    // Initialize memory management components
    physmem_init();
    paging_init();
    
    // Test memory allocation by allocating and freeing pages
    uint64 page1 = physmem_alloc_page();
    printf("Allocated page at: %p\n", (void*)page1);
    
    uint64 page2 = physmem_alloc_page();
    printf("Allocated page at: %p\n", (void*)page2);
    
    free_physical_page(page1);
    printf("Freed page at: %p\n", (void*)page1);
    
    uint64 free_pages = get_free_pages_count();
    printf("Free pages: %d\n", free_pages);
    
    printf("\nMemory management initialized successfully!\n");
    
    // Keep the kernel running
    while (1);
}
