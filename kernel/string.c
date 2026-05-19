// ============================================================================
// string.c - Fonctions chaînes/mémoire pour environnement freestanding
// ============================================================================

#include "kernel/types.h"

// ----------------------------------------------------------------------------
// memcpy - Copie n octets de src vers dest
// ----------------------------------------------------------------------------
void* memcpy(void *dest, const void *src, uint64 n)
{
    uint8 *d = (uint8*)dest;
    const uint8 *s = (const uint8*)src;
    
    for (uint64 i = 0; i < n; i++) {
        d[i] = s[i];
    }
    
    return dest;
}

// ----------------------------------------------------------------------------
// memset - Remplit n octets avec la valeur c
// ----------------------------------------------------------------------------
void* memset(void *s, int c, uint64 n)
{
    uint8 *p = (uint8*)s;
    
    for (uint64 i = 0; i < n; i++) {
        p[i] = (uint8)c;
    }
    
    return s;
}

// ----------------------------------------------------------------------------
// memmove - Copie avec gestion des zones qui se chevauchent
// ----------------------------------------------------------------------------
void* memmove(void *dest, const void *src, uint64 n)
{
    uint8 *d = (uint8*)dest;
    const uint8 *s = (const uint8*)src;
    
    if (d < s) {
        for (uint64 i = 0; i < n; i++) {
            d[i] = s[i];
        }
    } else {
        for (uint64 i = n; i > 0; i--) {
            d[i-1] = s[i-1];
        }
    }
    
    return dest;
}

// ----------------------------------------------------------------------------
// memcmp - Compare deux zones mémoire
// ----------------------------------------------------------------------------
int memcmp(const void *s1, const void *s2, uint64 n)
{
    const uint8 *p1 = (const uint8*)s1;
    const uint8 *p2 = (const uint8*)s2;
    
    for (uint64 i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return (p1[i] < p2[i]) ? -1 : 1;
        }
    }
    return 0;
}
