
// ============================================================================
// console.c - Console et fonctions d'affichage
// ============================================================================
// Fournit une interface d'affichage (printf) pour le noyau.
// ============================================================================

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
    
    // Gestion du signe négatif
    if (n < 0) {
        is_negative = 1;
        n = -n;
    }
    
    // Conversion inverse (chiffres les moins significatifs en premier)
    do {
        buffer[i++] = '0' + (n % 10);
        n /= 10;
    } while (n > 0);
    
    // Ajouter le signe négatif
    if (is_negative) {
        buffer[i++] = '-';
    }
    
    // Afficher dans l'ordre correct (du dernier au premier)
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
    uint64 x;
    
    // Pointeur vers les arguments (après fmt)
    char **arg = (char**)(&fmt + 1);
    
    for (p = fmt; *p; p++) {
        if (*p != '%') {
            // Caractère normal : afficher directement
            uart_putchar(*p);
            continue;
        }
        
        // Caractère '%' trouvé : lire le format
        p++;
        switch (*p) {
            case 's':
                // %s : chaîne de caractères
                s = *((char**)arg);
                arg++;
                print_string(s);
                break;
                
            case 'd':
                // %d : entier décimal
                i = *((int*)arg);
                arg++;
                print_int(i);
                break;
                
            case 'x':
                // %x : entier hexadécimal
                x = *((uint64*)arg);
                arg++;
                print_hex(x);
                break;
                
            case '%':
                // %% : afficher un '%'
                uart_putchar('%');
                break;
                
            default:
                // Format inconnu : afficher '%' et le caractère
                uart_putchar('%');
                uart_putchar(*p);
                break;
        }
    }
}
