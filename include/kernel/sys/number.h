// ===========================================================================
// number.h - Numéros des appels système du noyau Lithium
// ===========================================================================
// Ces numéros sont placés dans le registre a7 avant l'instruction ecall.
// La convention suit xv6 pour rester pédagogiquement cohérente.
// ===========================================================================

#ifndef KERNEL_SYS_NUMBER_H
#define KERNEL_SYS_NUMBER_H

#define SYS_fork    1   // Créer un processus fils
#define SYS_exit    2   // Terminer le processus courant
#define SYS_wait    3   // Attendre la fin d'un fils
#define SYS_read    4   // Lire depuis un descripteur de fichier
#define SYS_write   5   // Écrire vers un descripteur de fichier
#define SYS_getpid  6   // Obtenir le PID du processus courant
#define SYS_sleep   7   // Endormir le processus N ticks
#define SYS_yield   8   // Céder la main à l'ordonnanceur

#define NSYSCALLS   9   // Nombre total d'appels système

#endif
