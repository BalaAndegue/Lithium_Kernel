#ifndef KERNEL_IO_UART_H
#define KERNEL_IO_UART_H

#include "kernel/types.h"

// Adresse de base du UART sur QEMU RISC-V virt
#define UART_BASE   0x10000000

// Registres UART (offset par rapport à UART_BASE)
#define UART_TX_REG 0x00    // Transmit Holding Register (écriture)
#define UART_RX_REG 0x00    // Receive Holding Register (lecture)
#define UART_LSR_REG 0x05   // Line Status Register
#define UART_MCR_REG 0x04   // Modem Control Register

// Bits du Line Status Register
#define UART_LSR_THRE (1 << 5)  // Transmit Holding Register Empty
#define UART_LSR_DR   (1 << 0)  // Data Ready

// Initialize the UART controller
void uart_init(void);

// Send a single character
void uart_putchar(char c);

// Send a string (null-terminated)
void uart_puts(char *s);

// Send raw bytes with length
void uart_write(const uint8 *data, int len);

// Receive a character (blocking)
char uart_getchar(void);

// Check if character is available without blocking
int uart_available(void);

// Send hex dump of memory region (for debugging)
void uart_hexdump(const uint8 *data, int len);

// Send a newline
static inline void uart_newline(void)
{
    uart_putchar('\n');
}

#endif