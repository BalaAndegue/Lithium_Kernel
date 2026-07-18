
// UART driver for serial console

#include "kernel/io/uart.h"
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

// Send raw bytes with length
void uart_write(const uint8 *data, int len)
{
    for (int i = 0; i < len; i++) {
        uart_putchar(data[i]);
    }
}

// Receive a character (blocking)
char uart_getchar(void)
{
    // Wait for data to be available
    while (!(*(volatile uint8*)(UART_BASE + UART_LSR_REG) & UART_LSR_DR))
        ;  // Busy wait
    
    // Read the character
    return *(volatile uint8*)(UART_BASE + UART_RX_REG);
}

// Check if character is available without blocking
int uart_available(void)
{
    return (*(volatile uint8*)(UART_BASE + UART_LSR_REG) & UART_LSR_DR) ? 1 : 0;
}

// Send hex dump of memory region (for debugging)
void uart_hexdump(const uint8 *data, int len)
{
    uart_puts("HEXDUMP:\n");
    
    for (int i = 0; i < len; i++) {
        if (i % 16 == 0) {
            if (i > 0)
                uart_putchar('\n');
            // Print address
            uint64 addr = (uint64)&data[i];
            uart_puts("0x");
            for (int j = 60; j >= 0; j -= 4) {
                int digit = (addr >> j) & 0xF;
                uart_putchar(digit < 10 ? '0' + digit : 'A' + digit - 10);
            }
            uart_puts(": ");
        }
        
        // Print byte in hex
        uint8 byte = data[i];
        int high = (byte >> 4) & 0xF;
        int low = byte & 0xF;
        uart_putchar(high < 10 ? '0' + high : 'A' + high - 10);
        uart_putchar(low < 10 ? '0' + low : 'A' + low - 10);
        uart_putchar(' ');
    }
    uart_putchar('\n');
}

