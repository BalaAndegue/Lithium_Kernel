
// ============================================================================
// console.c - Console et fonctions d'affichage
// ============================================================================
// Fournit une interface d'affichage (printf) pour le noyau.
// ============================================================================

#include <stdarg.h>
#include "kernel/io/uart.h"
#include "kernel/io/console.h"
#include "kernel/types.h"

// ----------------------------------------------------------------------------
// print_string - Affiche une chaîne via l'UART
// ----------------------------------------------------------------------------
static void print_string(char *s)
{
    uart_puts(s);
}

// ----------------------------------------------------------------------------
// print_int - Convertit un entier en décimal et l'affiche
// ----------------------------------------------------------------------------
static void print_int(int n)
{
    char buffer[16];
    int i = 0;
    int is_negative = 0;
    
    if (n < 0) {
        is_negative = 1;
        n = -n;
    }
    
    do {
        buffer[i++] = '0' + (n % 10);
        n /= 10;
    } while (n > 0);
    
    if (is_negative) {
        buffer[i++] = '-';
    }
    
    while (i > 0) {
        uart_putchar(buffer[--i]);
    }
}

static void print_uint64(uint64 n)
{
    char buffer[32];
    int i = 0;

    if (n == 0) {
        uart_putchar('0');
        return;
    }

    while (n > 0) {
        buffer[i++] = '0' + (n % 10);
        n /= 10;
    }

    while (i > 0) {
        uart_putchar(buffer[--i]);
    }
}

static void print_int64(int64 n)
{
    uint64 value;
    char buffer[32];
    int i = 0;
    int is_negative = 0;

    if (n < 0) {
        is_negative = 1;
        value = -n;
    } else {
        value = n;
    }

    if (value == 0) {
        if (is_negative) {
            uart_putchar('-');
        }
        uart_putchar('0');
        return;
    }

    while (value > 0) {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }

    if (is_negative) {
        buffer[i++] = '-';
    }

    while (i > 0) {
        uart_putchar(buffer[--i]);
    }
}

// ----------------------------------------------------------------------------
// print_hex - Convertit un entier en hexadécimal et l'affiche
// ----------------------------------------------------------------------------
static void print_hex(uint64 n)
{
    char digits[] = "0123456789abcdef";
    char buffer[16];
    int i = 0;
    
    // Conversion inverse
    do {
        buffer[i++] = digits[n & 0xF];
        n >>= 4;
    } while (n > 0);
    
    // Préfixe "0x"
    uart_putchar('0');
    uart_putchar('x');
    
    // Afficher dans l'ordre correct
    while (i > 0) {
        uart_putchar(buffer[--i]);
    }
}

// ----------------------------------------------------------------------------
// console_putchar - Envoie un caractère sur la console UART
// ----------------------------------------------------------------------------
void console_putchar(char c)
{
    uart_putchar(c);
}

// ----------------------------------------------------------------------------
// printf - Affiche une chaîne formatée (version minimale)
// ----------------------------------------------------------------------------
// Formats supportés :
//   %s  - chaîne de caractères
//   %d  - entier décimal signé
//   %x  - entier hexadécimal (64 bits)
//   %%  - caractère '%' littéral
// ----------------------------------------------------------------------------
void printf(char *fmt, ...)
{
    char *p;
    char *s;
    int i;
    int64 li;
    uint64 x;
    void *ptr;
    va_list ap;

    va_start(ap, fmt);

    for (p = fmt; *p; p++) {
        if (*p != '%') {
            uart_putchar(*p);
            continue;
        }

        p++;
        switch (*p) {
            case 's':
                s = va_arg(ap, char*);
                print_string(s);
                break;

            case 'd':
                i = va_arg(ap, int);
                print_int(i);
                break;

            case 'u':
                x = va_arg(ap, uint64);
                print_uint64(x);
                break;

            case 'x':
                x = va_arg(ap, uint64);
                print_hex(x);
                break;

            case 'p':
                ptr = va_arg(ap, void*);
                print_hex((uint64)ptr);
                break;

            case 'l':
                p++;
                switch (*p) {
                    case 'd':
                        li = va_arg(ap, int64);
                        print_int64(li);
                        break;
                    case 'u':
                        x = va_arg(ap, uint64);
                        print_uint64(x);
                        break;
                    case 'x':
                        x = va_arg(ap, uint64);
                        print_hex(x);
                        break;
                    default:
                        uart_putchar('%');
                        uart_putchar('l');
                        uart_putchar(*p);
                        break;
                }
                break;

            case 'z':
                p++;
                if (*p == 'u') {
                    x = va_arg(ap, uint64);
                    print_uint64(x);
                } else {
                    uart_putchar('%');
                    uart_putchar('z');
                    uart_putchar(*p);
                }
                break;

            case '%':
                uart_putchar('%');
                break;

            default:
                uart_putchar('%');
                uart_putchar(*p);
                break;
        }
    }

    va_end(ap);
}
