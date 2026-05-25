// ===========================================================================
// trap.h - Interface du gestionnaire de pièges RISC-V
// ===========================================================================

#ifndef KERNEL_TRAP_H
#define KERNEL_TRAP_H

// Initialiser le vecteur de pièges du kernel (stvec)
void trap_init(void);

// Gestionnaire de pièges en mode kernel
// Appelé quand une exception/interruption survient en mode superviseur
void kerneltrap(void);

// Gestionnaire de pièges en mode utilisateur
// Appelé depuis le trampoline quand un piège survient en mode user
void usertrap(void);

// Retour vers l'espace utilisateur après gestion d'un piège
void usertrapret(void);

#endif
