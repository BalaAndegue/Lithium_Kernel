
// ============================================================================
// uart.c - Pilote UART (Universal Asynchronous Receiver/Transmitter)
// ============================================================================
// Ce pilote contrôle la console série.
// Sur QEMU RISC-V virt, l'UART est un NS16550a à l'adresse 0x10000000.
// ============================================================================

#include "kernel/io/uart.h"
#include "kernel/io/uart_defs.h"
#include "kernel/types.h"

// ----------------------------------------------------------------------------
// uart_init - Initialise le contrôleur UART
// ----------------------------------------------------------------------------
// Sur QEMU, l'UART est déjà configuré par défaut.
// Cette fonction est principalement pour la compatibilité.
void uart_init(void)
{
    // L'adresse de base de l'UART est définie dans uart.h
    // Pas de configuration complexe nécessaire pour QEMU
}

// ----------------------------------------------------------------------------
// uart_putchar - Envoie un caractère sur la console série
// ----------------------------------------------------------------------------
// Paramètre :
//   c - Le caractère à envoyer (converti en unsigned char)
// ----------------------------------------------------------------------------
void uart_putchar(char c)
{
    // Attendre que le registre de transmission soit vide
    // UART_LSR_REG = Line Status Register
    // UART_LSR_THRE = Transmit Holding Register Empty (bit 5)
    while (!(*(volatile uint8*)(UART_BASE + UART_LSR_REG) & UART_LSR_THRE))
        ;  // Boucle d'attente
    
    // Écrire le caractère dans le registre de transmission
    *(volatile uint8*)(UART_BASE + UART_TX_REG) = c;
}

// ----------------------------------------------------------------------------
// uart_puts - Envoie une chaîne de caractères sur la console
// ----------------------------------------------------------------------------
// Paramètre :
//   s - Pointeur vers la chaîne terminée par '\0'
// ----------------------------------------------------------------------------
void uart_puts(char *s)
{
    // Parcourir la chaîne jusqu'au caractère nul
    while (*s) {
        uart_putchar(*s);
        s++;
    }
}
