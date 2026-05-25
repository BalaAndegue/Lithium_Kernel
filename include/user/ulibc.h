// ===========================================================================
// ulibc.h - Bibliothèque C minimale pour l'espace utilisateur Lithium
// ===========================================================================

#ifndef USER_ULIBC_H
#define USER_ULIBC_H

#include "kernel/types.h"
#include "user/syscall.h"

// ---------------------------------------------------------------------------
// Sortie formatée
// ---------------------------------------------------------------------------
void u_puts(const char *s);
void u_putchar(char c);
void u_printf(const char *fmt, ...);

// ---------------------------------------------------------------------------
// Manipulation de chaînes
// ---------------------------------------------------------------------------
int    u_strlen(const char *s);
char  *u_strcpy(char *dst, const char *src);
int    u_strcmp(const char *a, const char *b);
void  *u_memset(void *dst, int c, uint32 n);
void  *u_memcpy(void *dst, const void *src, uint32 n);

// ---------------------------------------------------------------------------
// Conversion
// ---------------------------------------------------------------------------
int    u_atoi(const char *s);
void   u_itoa(int n, char *buf);

#endif
