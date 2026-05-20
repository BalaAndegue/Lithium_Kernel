
#include "kernel/io/uart.h"
#include "kernel/io/console.h"
#include "kernel/mem/physmem.h"
#include "kernel/mem/paging.h"
#include "kernel/proc/control.h"
#include "kernel/proc/scheduler.h"

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
    printf("Free pages: %u\n", free_pages);
    
    printf("\nMemory management initialized successfully!\n");
    
    // Keep the kernel running

    printf("\n--- Bloc 3: Process Management ---\n");
    proc_init();
    
    printf("\n--- Test: fork() ---\n");
    int pid = fork();
    if (pid == 0) {
        printf("Child process: PID=%d\n", myproc()->pid);
        exit(0);
    } else if (pid > 0) {
        printf("Parent: created child PID=%d\n", pid);
        int status;
        wait((uint64)&status);
        printf("Parent: child exited with status %d\n", status);
    }
    
    printf("\nBloc 3: SUCCESS!\n");
    printf("Starting scheduler...\n");
    
    // L'ordonnanceur boucle infiniment
    scheduler();
    
    // Normalement, on n'arrive jamais ici
    // Mais par sécurité, on ajoute une boucle infinie
    printf("ERROR: scheduler returned! Halting...\n");
    while (1) {
        asm volatile("wfi");
    }

}

