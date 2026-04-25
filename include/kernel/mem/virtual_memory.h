
// ============================================================================
// virtual_memory.h - Pagination SV39 (RISC-V 64 bits)
// ============================================================================
// Définit les structures et constantes pour la mémoire virtuelle.
// SV39 = 39 bits d'adresse virtuelle, 3 niveaux de tables (9+9+9+12)
// ============================================================================



#ifndef KERNEL_MEM_VIRTUAL_MEMORY_H
#define KERNEL_MEM_VIRTUAL_MEMORY_H

#include "kernel/types.h"

// Entrée de table de pages
typedef uint64 pte_t;

// Bits de permission
#define PTE_V (1 << 0)   // Valid
#define PTE_R (1 << 1)   // Read
#define PTE_W (1 << 2)   // Write
#define PTE_X (1 << 3)   // Execute
#define PTE_U (1 << 4)   // User
#define PTE_G (1 << 5)   // Global
#define PTE_A (1 << 6)   // Accessed
#define PTE_D (1 << 7)   // Dirty

// Masques
#define PTE_ADDR_MASK   0x3FFFFFFFFC00UL  // Bits [53:10]
#define VIRT_ADDR_MASK  0xFFFFFFFFFFFFUL

// Extractions
#define PTE_ADDR(pte)   ((pte) & PTE_ADDR_MASK)
#define MAKE_PTE(phys_addr, flags) (((phys_addr) & PTE_ADDR_MASK) | (flags))

// Indices VPN pour SV39
#define VPN2(va) (((va) >> 30) & 0x1FF)  // Bits 38-30
#define VPN1(va) (((va) >> 21) & 0x1FF)  // Bits 29-21
#define VPN0(va) (((va) >> 12) & 0x1FF)  // Bits 20-12

#endif
