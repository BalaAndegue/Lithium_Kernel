// ===========================================================================
// riscv_defs.h - Constantes des registres de contrôle RISC-V (CSR)
// ===========================================================================

#ifndef RISCV_DEFS_H
#define RISCV_DEFS_H

// ---------------------------------------------------------------------------
// Bits du registre sstatus
// ---------------------------------------------------------------------------
#define SSTATUS_SIE   (1L << 1)   // Interruptions superviseur activées
#define SSTATUS_SPIE  (1L << 5)   // SIE avant le dernier piège
#define SSTATUS_SPP   (1L << 8)   // Mode avant le piège (0=user, 1=superviseur)

// ---------------------------------------------------------------------------
// Bits du registre sie (supervisor interrupt enable)
// ---------------------------------------------------------------------------
#define SIE_SEIE  (1L << 9)   // Interruptions externes
#define SIE_STIE  (1L << 5)   // Interruptions timer
#define SIE_SSIE  (1L << 1)   // Interruptions logicielles

// ---------------------------------------------------------------------------
// Causes de piège (registre scause)
// Le bit 63 à 1 = interruption, à 0 = exception
// ---------------------------------------------------------------------------
#define SCAUSE_INTERRUPT_BIT           (1L << 63)

#define SCAUSE_TIMER_INTERRUPT         (SCAUSE_INTERRUPT_BIT | 5)
#define SCAUSE_EXTERNAL_INTERRUPT      (SCAUSE_INTERRUPT_BIT | 9)
#define SCAUSE_SOFTWARE_INTERRUPT      (SCAUSE_INTERRUPT_BIT | 1)

#define SCAUSE_INSTRUCTION_MISALIGNED  0
#define SCAUSE_INSTRUCTION_FAULT       1
#define SCAUSE_ILLEGAL_INSTRUCTION     2
#define SCAUSE_BREAKPOINT              3
#define SCAUSE_LOAD_MISALIGNED         4
#define SCAUSE_LOAD_FAULT              5
#define SCAUSE_STORE_MISALIGNED        6
#define SCAUSE_STORE_FAULT             7
#define SCAUSE_ECALL_USER              8   // appel système depuis user mode
#define SCAUSE_ECALL_SUPERVISOR        9
#define SCAUSE_INSTRUCTION_PAGE_FAULT  12
#define SCAUSE_LOAD_PAGE_FAULT         13
#define SCAUSE_STORE_PAGE_FAULT        15

// ---------------------------------------------------------------------------
// Adresses PLIC (Platform-Level Interrupt Controller) — QEMU virt RISC-V
// ---------------------------------------------------------------------------
#define PLIC_BASE       0x0C000000L
#define PLIC_PRIORITY   (PLIC_BASE + 0x0)
#define PLIC_PENDING    (PLIC_BASE + 0x1000)
#define PLIC_SENABLE    (PLIC_BASE + 0x2080)    // Enable superviseur hart 0
#define PLIC_SPRIORITY  (PLIC_BASE + 0x201000)  // Seuil priorité superviseur hart 0
#define PLIC_SCLAIM     (PLIC_BASE + 0x201004)  // Claim/Complete superviseur hart 0

#define UART0_IRQ    10
#define VIRTIO0_IRQ   1

#endif
