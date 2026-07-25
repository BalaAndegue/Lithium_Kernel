#ifndef KERNEL_IO_PRINTK_H
#define KERNEL_IO_PRINTK_H

#include <stdarg.h>

// Printf-like function for kernel logging via UART
// Supports basic format specifiers: %d (int), %x (hex), %s (string), %c (char), %%
// Returns number of characters printed, or -1 on error
int printk(const char *fmt, ...);
int vprintk(const char *fmt, va_list ap);

// printf alias for compatibility
#define printf printk

#endif
