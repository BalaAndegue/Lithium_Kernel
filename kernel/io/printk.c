#include "kernel/io/printk.h"
#include "kernel/io/uart.h"
#include "kernel/types.h"
#include <stdarg.h>

// Forward declaration
static void printk_write_int(int n, int base, int width, char pad);
static void printk_write_uint(uint32 n, int base, int width, char pad);

int vprintk(const char *fmt, va_list ap)
{
    int count = 0;

    while (*fmt) {
        if (*fmt != '%') {
            uart_putchar(*fmt);
            count++;
            fmt++;
            continue;
        }

        fmt++;  // Skip '%'
        if (*fmt == '\0')
            break;

        char pad = ' ';
        int width = 0;
        if (*fmt == '0') {
            pad = '0';
            fmt++;
        }

        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }

        switch (*fmt) {
        case '%':
            uart_putchar('%');
            count++;
            break;

        case 'd':
        case 'i': {
            int val = va_arg(ap, int);
            if (val < 0) {
                uart_putchar('-');
                count++;
                val = -val;
            }
            printk_write_int(val, 10, width, pad);
            count += (width > 0) ? width : ((val == 0) ? 1 : 0);
            if (width == 0) {
                int temp = val;
                while (temp > 0) {
                    count++;
                    temp /= 10;
                }
                if (val == 0) count = 1;
            }
            break;
        }

        case 'u': {
            uint32 val = va_arg(ap, uint32);
            printk_write_uint(val, 10, width, pad);
            count += (width > 0) ? width : 1;
            break;
        }

        case 'x':
        case 'X': {
            uint32 val = va_arg(ap, uint32);
            printk_write_uint(val, 16, width, pad);
            count += (width > 0) ? width : 1;
            break;
        }

        case 's': {
            char *str = va_arg(ap, char *);
            if (!str) str = "(null)";
            while (*str) {
                uart_putchar(*str);
                count++;
                str++;
            }
            break;
        }

        case 'c': {
            char c = (char)va_arg(ap, int);
            uart_putchar(c);
            count++;
            break;
        }

        case 'p': {
            uart_putchar('0');
            uart_putchar('x');
            count += 2;
            uint32 val = va_arg(ap, uint32);
            printk_write_uint(val, 16, 8, '0');
            count += 8;
            break;
        }

        default:
            uart_putchar('%');
            uart_putchar(*fmt);
            count += 2;
            break;
        }

        fmt++;
    }

    return count;
}

int printk(const char *fmt, ...)
{
    va_list ap;
    int count;

    va_start(ap, fmt);
    count = vprintk(fmt, ap);
    va_end(ap);

    return count;
}

// Write integer in given base (used for %d)
static void printk_write_int(int n, int base, int width, char pad)
{
    // Recursive approach: build from least to most significant digit
    if (n == 0) {
        if (width > 0) {
            for (int i = 1; i < width; i++) {
                uart_putchar(pad);
            }
        }
        uart_putchar('0');
        return;
    }
    
    // Handle negative for base 10
    if (n < 0 && base == 10) {
        uart_putchar('-');
        n = -n;
    }
    
    // Recursive write
    if (n / base > 0)
        printk_write_int(n / base, base, 0, ' ');
    
    int digit = n % base;
    if (digit < 10)
        uart_putchar('0' + digit);
    else
        uart_putchar('a' + digit - 10);
}

// Write unsigned integer in given base (used for %x, %u)
static void printk_write_uint(uint32 n, int base, int width, char pad)
{
    if (n == 0) {
        if (width > 0) {
            for (int i = 1; i < width; i++) {
                uart_putchar(pad);
            }
        }
        uart_putchar('0');
        return;
    }
    
    // Recursive write
    if (n / base > 0)
        printk_write_uint(n / base, base, 0, ' ');
    
    int digit = n % base;
    if (digit < 10)
        uart_putchar('0' + digit);
    else
        uart_putchar('a' + digit - 10);
}
