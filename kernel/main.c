
// ============================================================================
// main.c - Point d'entrée principal du noyau
// ============================================================================
// Cette fonction est appelée depuis entry.S après l'initialisation
// de la pile. Elle initialise les sous-systèmes puis boucle.
// ============================================================================

#include "kernel/io/uart.h"
#include "kernel/io/console.h"

// ----------------------------------------------------------------------------
// kernel_main - Fonction principale du noyau
// ----------------------------------------------------------------------------
void kernel_main(void)
{
    // 1. Initialiser l'UART (console série)
    uart_init();
    
    // 2. Afficher le message de bienvenue
    printf("Lithium Kernel starting...\n");
    printf("UART initialized at address 0x10000000\n");
    printf("========================================\n");
    printf("\n");
    printf("Bloc 1: Boot + UART - SUCCESS !\n");
    printf("\n");
    printf("Ready for next steps:\n");
    printf("  - Bloc 2: Memory management\n");
    printf("  - Bloc 3: Processes\n");
    printf("  - Bloc 4: System calls\n");
    printf("\n");
    
    // 3. Boucle infinie (le noyau tourne ici)
    while (1) {
        // Pour l'instant, on ne fait rien
        // Plus tard, ce sera l'ordonnanceur (scheduler)
    }
}
