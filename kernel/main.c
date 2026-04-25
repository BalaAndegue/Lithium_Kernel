
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
    
    printf("\nMemory management initialized successfully!\n");
    
    // Keep the kernel running
    while (1);
}
