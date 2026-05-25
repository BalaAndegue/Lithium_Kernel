// ===========================================================================
// plic.c - Contrôleur d'interruptions PLIC (Platform-Level Interrupt Controller)
// ===========================================================================
//
// Le PLIC est le composant RISC-V qui reçoit toutes les interruptions
// matérielles (UART, disque VirtIO, etc.) et les route vers les cœurs CPU.
// Sur QEMU virt, il est mappé en mémoire à 0x0C000000.
// ===========================================================================

#include "kernel/types.h"
#include "kernel/trap/plic.h"
#include "kernel/io/console.h"
#include "riscv/riscv_defs.h"

// Accès direct à la mémoire mappée du PLIC
// On lit/écrit des uint32 à des adresses fixes
#define PLIC_REG(addr) (*((volatile uint32 *)(uint64)(addr)))

// ---------------------------------------------------------------------------
// plic_init - Activer les interruptions UART et VirtIO sur le hart 0
// ---------------------------------------------------------------------------
void plic_init(void)
{
    // Fixer la priorité de chaque source à 1 (> 0 = activée)
    PLIC_REG(PLIC_PRIORITY + UART0_IRQ   * 4) = 1;
    PLIC_REG(PLIC_PRIORITY + VIRTIO0_IRQ * 4) = 1;

    // Activer les sources IRQ dans le registre enable du superviseur hart 0
    // Chaque bit correspond à un numéro IRQ
    PLIC_REG(PLIC_SENABLE) = (1 << UART0_IRQ) | (1 << VIRTIO0_IRQ);

    // Fixer le seuil de priorité à 0 : toutes les interruptions passent
    PLIC_REG(PLIC_SPRIORITY) = 0;

    printf("plic_init: UART IRQ=%d et VirtIO IRQ=%d activés\n",
           UART0_IRQ, VIRTIO0_IRQ);
}

// ---------------------------------------------------------------------------
// plic_claim - Demander au PLIC quelle interruption est en attente
// ---------------------------------------------------------------------------
// Retourne le numéro IRQ de l'interruption la plus prioritaire,
// ou 0 si aucune interruption n'est en attente.
// Après avoir lu ce registre, on "possède" l'interruption et on doit
// appeler plic_complete() une fois traitée.
// ---------------------------------------------------------------------------
int plic_claim(void)
{
    return (int)PLIC_REG(PLIC_SCLAIM);
}

// ---------------------------------------------------------------------------
// plic_complete - Signaler au PLIC que l'interruption a été traitée
// ---------------------------------------------------------------------------
void plic_complete(int irq)
{
    PLIC_REG(PLIC_SCLAIM) = (uint32)irq;
}
