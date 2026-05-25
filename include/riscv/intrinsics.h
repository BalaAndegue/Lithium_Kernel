// ===========================================================================
// intrinsics.h - Accès inline aux registres CSR RISC-V
// ===========================================================================

#ifndef RISCV_INTRINSICS_H
#define RISCV_INTRINSICS_H

#include "kernel/types.h"

// Macro générique : lire un CSR
#define r_csr(reg) ({                         \
    uint64 _val;                              \
    asm volatile("csrr %0, " #reg : "=r"(_val)); \
    _val;                                     \
})

// Macro générique : écrire un CSR
#define w_csr(reg, val) \
    asm volatile("csrw " #reg ", %0" : : "r"((uint64)(val)))

// Macro générique : set bits dans un CSR
#define s_csr(reg, val) \
    asm volatile("csrs " #reg ", %0" : : "r"((uint64)(val)))

// Macro générique : clear bits dans un CSR
#define c_csr(reg, val) \
    asm volatile("csrc " #reg ", %0" : : "r"((uint64)(val)))

// ---------------------------------------------------------------------------
// Registres les plus utilisés dans le kernel
// ---------------------------------------------------------------------------
static inline uint64 r_sstatus(void)  { return r_csr(sstatus); }
static inline void   w_sstatus(uint64 v) { w_csr(sstatus, v); }

static inline uint64 r_scause(void)   { return r_csr(scause); }
static inline uint64 r_sepc(void)     { return r_csr(sepc); }
static inline void   w_sepc(uint64 v) { w_csr(sepc, v); }
static inline uint64 r_stval(void)    { return r_csr(stval); }

static inline uint64 r_sie(void)      { return r_csr(sie); }
static inline void   w_sie(uint64 v)  { w_csr(sie, v); }

static inline uint64 r_satp(void)     { return r_csr(satp); }
static inline void   w_satp(uint64 v) { w_csr(satp, v); }

static inline void   w_stvec(uint64 v) { w_csr(stvec, v); }

// Lire le numéro de hart (cœur) courant
static inline uint64 r_tp(void) {
    uint64 v;
    asm volatile("mv %0, tp" : "=r"(v));
    return v;
}

// Activer/désactiver les interruptions superviseur
static inline void intr_on(void)  { s_csr(sstatus, (1L << 1)); }
static inline void intr_off(void) { c_csr(sstatus, (1L << 1)); }
static inline int  intr_get(void) { return (r_sstatus() >> 1) & 1; }

#endif
