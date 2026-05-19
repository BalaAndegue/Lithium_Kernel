#ifndef KERNEL_STRING_H
#define KERNEL_STRING_H

#include "kernel/types.h"


void* memcpy(void *dest, const void *src, uint64 n);
void* memset(void *s, int c, uint64 n);  
void* memmove(void *dest, const void *src, uint64 n);
int memcmp(const void *s1, const void *s2, uint64 n);

#endif // KERNEL_STRING_H