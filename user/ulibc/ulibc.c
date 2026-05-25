// ===========================================================================
// ulibc.c - Bibliothèque C minimale pour l'espace utilisateur Lithium
// ===========================================================================
//
// Cette bibliothèque fournit aux programmes utilisateur les fonctions de base
// dont ils ont besoin sans dépendre de la glibc.
// Toutes les sorties passent par le syscall write (fd=1).
// ===========================================================================

#include "kernel/types.h"
#include "user/ulibc.h"
#include "user/fnctl.h"

// ---------------------------------------------------------------------------
// u_putchar - Écrire un seul caractère sur stdout
// ---------------------------------------------------------------------------
void u_putchar(char c)
{
    u_write(STDOUT_FILENO, &c, 1);
}

// ---------------------------------------------------------------------------
// u_puts - Écrire une chaîne suivie d'un saut de ligne
// ---------------------------------------------------------------------------
void u_puts(const char *s)
{
    while (*s)
        u_putchar(*s++);
    u_putchar('\n');
}

// ---------------------------------------------------------------------------
// u_strlen - Longueur d'une chaîne
// ---------------------------------------------------------------------------
int u_strlen(const char *s)
{
    int n = 0;
    while (s[n]) n++;
    return n;
}

// ---------------------------------------------------------------------------
// u_strcpy - Copier une chaîne
// ---------------------------------------------------------------------------
char *u_strcpy(char *dst, const char *src)
{
    char *d = dst;
    while ((*d++ = *src++))
        ;
    return dst;
}

// ---------------------------------------------------------------------------
// u_strcmp - Comparer deux chaînes
// ---------------------------------------------------------------------------
int u_strcmp(const char *a, const char *b)
{
    while (*a && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

// ---------------------------------------------------------------------------
// u_memset / u_memcpy
// ---------------------------------------------------------------------------
void *u_memset(void *dst, int c, uint32 n)
{
    unsigned char *d = dst;
    while (n--) *d++ = (unsigned char)c;
    return dst;
}

void *u_memcpy(void *dst, const void *src, uint32 n)
{
    unsigned char       *d = dst;
    const unsigned char *s = src;
    while (n--) *d++ = *s++;
    return dst;
}

// ---------------------------------------------------------------------------
// u_itoa - Convertir un entier en chaîne décimale
// ---------------------------------------------------------------------------
void u_itoa(int n, char *buf)
{
    if (n < 0) {
        *buf++ = '-';
        n = -n;
    }

    char tmp[12];
    int  i = 0;

    if (n == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }

    while (n > 0) {
        tmp[i++] = '0' + (n % 10);
        n /= 10;
    }

    // Inverser
    for (int j = 0; j < i; j++)
        buf[j] = tmp[i - 1 - j];
    buf[i] = '\0';
}

// ---------------------------------------------------------------------------
// u_atoi - Convertir une chaîne décimale en entier
// ---------------------------------------------------------------------------
int u_atoi(const char *s)
{
    int n   = 0;
    int neg = 0;

    if (*s == '-') { neg = 1; s++; }
    while (*s >= '0' && *s <= '9')
        n = n * 10 + (*s++ - '0');

    return neg ? -n : n;
}

// ---------------------------------------------------------------------------
// u_printf - Sortie formatée minimale (%d, %s, %c, %x)
// ---------------------------------------------------------------------------
void u_printf(const char *fmt, ...)
{
    // Accès aux arguments variadiques manuellement (pas de <stdarg.h>)
    uint64 *args = (uint64 *)&fmt + 1;
    int    arg   = 0;
    char   buf[20];

    for (const char *p = fmt; *p; p++) {
        if (*p != '%') {
            u_putchar(*p);
            continue;
        }
        p++;
        switch (*p) {
        case 'd': {
            int n = (int)args[arg++];
            u_itoa(n, buf);
            for (char *b = buf; *b; b++) u_putchar(*b);
            break;
        }
        case 's': {
            const char *s = (const char *)args[arg++];
            if (s == (void *)0) s = "(null)";
            while (*s) u_putchar(*s++);
            break;
        }
        case 'c':
            u_putchar((char)args[arg++]);
            break;
        case 'x': {
            uint64 n = args[arg++];
            char hex[17];
            int  i = 0;
            if (n == 0) { u_putchar('0'); break; }
            while (n > 0) {
                int d = n & 0xf;
                hex[i++] = d < 10 ? '0' + d : 'a' + d - 10;
                n >>= 4;
            }
            for (int j = i - 1; j >= 0; j--)
                u_putchar(hex[j]);
            break;
        }
        case '%':
            u_putchar('%');
            break;
        default:
            u_putchar('%');
            u_putchar(*p);
            break;
        }
    }
}
