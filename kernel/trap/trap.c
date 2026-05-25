// ===========================================================================
// trap.c - Gestionnaire de pièges RISC-V (exceptions et interruptions)
// ===========================================================================
//
// Un "piège" (trap) en RISC-V désigne tout événement qui détourne
// l'exécution normale : appel système, faute de page, interruption timer...
// Ce fichier gère ces événements pour le mode kernel (superviseur).
// ===========================================================================

#include <stddef.h>
#include "kernel/types.h"
#include "kernel/trap/trap.h"
#include "kernel/trap/plic.h"
#include "kernel/proc/process.h"
#include "kernel/proc/scheduler.h"
#include "kernel/proc/globals.h"
#include "kernel/io/console.h"
#include "riscv/riscv_defs.h"
#include "riscv/intrinsics.h"

// Déclaré dans syscall.c — dispatch vers le bon appel système
extern void syscall(void);

// ---------------------------------------------------------------------------
// trap_init - Configurer stvec pour pointer sur kerneltrap
// ---------------------------------------------------------------------------
// stvec contient l'adresse du gestionnaire de pièges.
// On utilise le mode "direct" (bits[1:0] = 00) : tous les pièges
// arrivent à la même adresse (kerneltrap).
// ---------------------------------------------------------------------------
void trap_init(void)
{
    // Pointer stvec sur notre fonction C kerneltrap
    // Mode direct : bits 0-1 = 0b00
    w_stvec((uint64)kerneltrap & ~0x3UL);

    // Activer les interruptions externes et timer en mode superviseur
    w_sie(r_sie() | SIE_SEIE | SIE_STIE);

    printf("trap_init: stvec configuré sur kerneltrap\n");
}

// ---------------------------------------------------------------------------
// Noms lisibles des causes d'exception (pour les messages d'erreur)
// ---------------------------------------------------------------------------
static const char *exception_names[] = {
    "instruction misaligned",   // 0
    "instruction fault",        // 1
    "illegal instruction",      // 2
    "breakpoint",               // 3
    "load misaligned",          // 4
    "load fault",               // 5
    "store misaligned",         // 6
    "store fault",              // 7
    "ecall from user",          // 8
    "ecall from supervisor",    // 9
    "reserved",                 // 10
    "reserved",                 // 11
    "instruction page fault",   // 12
    "load page fault",          // 13
    "reserved",                 // 14
    "store page fault",         // 15
};

// ---------------------------------------------------------------------------
// kerneltrap - Gestionnaire principal des pièges en mode kernel
// ---------------------------------------------------------------------------
// Appelé directement par le CPU quand un piège survient en mode superviseur.
// On lit scause pour savoir pourquoi on est ici.
// ---------------------------------------------------------------------------
void kerneltrap(void)
{
    uint64 scause  = r_scause();
    uint64 sepc    = r_sepc();
    uint64 stval   = r_stval();

    if (scause & SCAUSE_INTERRUPT_BIT) {
        // ----------------------------------------------------------------
        // C'est une interruption (bit 63 = 1)
        // ----------------------------------------------------------------
        uint64 cause = scause & ~SCAUSE_INTERRUPT_BIT;

        if (cause == 5) {
            // Interruption timer : laisser la main à un autre processus
            yield();

        } else if (cause == 9) {
            // Interruption externe : demander au PLIC quel périphérique
            int irq = plic_claim();
            if (irq == UART0_IRQ) {
                // L'UART a reçu un caractère — à gérer dans console.c
                printf("[trap] interruption UART reçue\n");
            } else if (irq == VIRTIO0_IRQ) {
                // Le disque VirtIO a fini une opération
                printf("[trap] interruption VirtIO reçue\n");
            }
            if (irq > 0)
                plic_complete(irq);

        } else {
            printf("kerneltrap: interruption inconnue cause=%lu sepc=%lx\n",
                   cause, sepc);
        }

    } else {
        // ----------------------------------------------------------------
        // C'est une exception (bit 63 = 0)
        // ----------------------------------------------------------------

        if (scause == SCAUSE_ECALL_USER || scause == SCAUSE_ECALL_SUPERVISOR) {
            // Appel système : dispatcher vers syscall.c
            // On avance sepc de 4 pour reprendre après l'instruction ecall
            w_sepc(sepc + 4);
            syscall();

        } else {
            // Toute autre exception est fatale pour l'instant
            const char *name = (scause < 16) ? exception_names[scause] : "inconnue";
            printf("kerneltrap: exception '%s' (scause=%lu)\n", name, scause);
            printf("  sepc=%lx  stval=%lx\n", sepc, stval);

            if (current_process != NULL) {
                printf("  processus: pid=%d nom=%s\n",
                       current_process->pid, current_process->name);
            }

            // Bloquer le kernel — un vrai noyau ferait panic ici
            printf("kernel panic: exception non gérée, arrêt.\n");
            while (1)
                ;
        }
    }
}

// ---------------------------------------------------------------------------
// usertrap / usertrapret - Stubs pour quand l'espace user sera implémenté
// ---------------------------------------------------------------------------
void usertrap(void)
{
    // Quand on aura un espace utilisateur, cette fonction lira le trapframe
    // du processus courant et dispatchera selon scause.
    printf("usertrap: non encore implémenté\n");
    while (1)
        ;
}

void usertrapret(void)
{
    printf("usertrapret: non encore implémenté\n");
    while (1)
        ;
}
