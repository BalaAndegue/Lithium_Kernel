
// UART driver for serial console

#include "kernel/io/uart.h"
#include "kernel/io/uart_defs.h"
#include "kernel/types.h"

// Initialize the UART controller
void uart_init(void)
{
    // UART base address is defined in uart.h
    // No complex configuration needed for QEMU
}

// Send a character to the serial console
void uart_putchar(char c)
{
    // Wait for transmit holding register to be empty
    while (!(*(volatile uint8*)(UART_BASE + UART_LSR_REG) & UART_LSR_THRE))
        ;  // Busy wait
    
    // Write the character to the transmit register
    *(volatile uint8*)(UART_BASE + UART_TX_REG) = c;
}

// Send a string to the serial console
void uart_puts(char *s)
{
    // Loop until null terminator
    while (*s) {
        uart_putchar(*s);
        s++;
    }
}
