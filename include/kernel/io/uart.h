#ifndef KERNEL_IO_UART_H
#define KERNEL_IO_UART_H

// Adresse de base du UART sur QEMU RISC-V virt
#define UART_BASE   0x10000000

// Registres UART (offset par rapport à UART_BASE)
#define UART_TX_REG 0x00    // Transmit Holding Register (écriture)
#define UART_LSR_REG 0x05   // Line Status Register (lecture)

// Bits du Line Status Register
#define UART_LSR_THRE (1 << 5)  // Transmit Holding Register Empty

void uart_init(void);
void uart_putchar(char c);
void uart_puts(char *s);

#endif