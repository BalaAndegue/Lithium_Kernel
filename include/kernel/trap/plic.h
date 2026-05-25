// ===========================================================================
// plic.h - Interface du contrôleur d'interruptions PLIC RISC-V
// ===========================================================================

#ifndef KERNEL_TRAP_PLIC_H
#define KERNEL_TRAP_PLIC_H

#include "kernel/types.h"

// Initialiser le PLIC : activer UART et VirtIO, fixer les priorités
void plic_init(void);

// Demander quelle interruption est en attente (claim)
// Retourne le numéro IRQ, ou 0 si aucune
int plic_claim(void);

// Signaler que l'interruption IRQ a été traitée (complete)
void plic_complete(int irq);

#endif
