
// ===========================================================================
// process.h - Structure et états d'un processus
// ===========================================================================

#ifndef KERNEL_PROC_PROCESS_H
#define KERNEL_PROC_PROCESS_H

#include "kernel/types.h"

// ---------------------------------------------------------------------------
// États possibles d'un processus
// ---------------------------------------------------------------------------
enum proc_state {
    PROC_UNUSED = 0,    // Pas utilisé (libre)
    PROC_USED,          // Alloué mais pas encore prêt
    PROC_SLEEPING,      // Dort (attendant un événement)
    PROC_RUNNABLE,      // Prêt à s'exécuter
    PROC_RUNNING,       // En cours d'exécution
    PROC_ZOMBIE         // Terminé, attendant que son parent le nettoie
};

// ---------------------------------------------------------------------------
// Contexte du processeur - registres sauvegardés (partie preserved)
// ---------------------------------------------------------------------------
// Ces registres doivent être sauvegardés quand on change de processus
struct context {
    uint64 ra;      // Adresse de retour
    uint64 sp;      // Pointeur de pile
    uint64 s0;      // Registres préservés (6 registres)
    uint64 s1;
    uint64 s2;
    uint64 s3;
    uint64 s4;
    uint64 s5;
    uint64 s6;
    uint64 s7;
    uint64 s8;
    uint64 s9;
    uint64 s10;
    uint64 s11;
};



// ---------------------------------------------------------------------------
// Trapframe - sauvegarde de TOUS les registres utilisateur
// ---------------------------------------------------------------------------
// Utilisé pour sauvegarder l'état complet lors d'une interruption ou appel système
struct trapframe {
    uint64 kernel_satp;     // Table des pages du kernel
    uint64 kernel_sp;       // Pile du kernel
    uint64 kernel_trap;     // Fonction de gestion des pièges
    uint64 epc;             // Adresse du code quand le piège a eu lieu
    uint64 kernel_hartid;   // ID du processeur
    uint64 ra;              // Registres de valeur temporaire
    uint64 sp;
    uint64 gp;
    uint64 tp;
    uint64 t0;
    uint64 t1;
    uint64 t2;
    uint64 s0;              // Registres préservés
    uint64 s1;
    uint64 a0;              // Registres d'arguments et de retour
    uint64 a1;
    uint64 a2;
    uint64 a3;
    uint64 a4;
    uint64 a5;
    uint64 a6;
    uint64 a7;
    uint64 s2;
    uint64 s3;
    uint64 s4;
    uint64 s5;
    uint64 s6;
    uint64 s7;
    uint64 s8;
    uint64 s9;
    uint64 s10;
    uint64 s11;
    uint64 t3;              // Plus de registres temporaires
    uint64 t4;
    uint64 t5;
    uint64 t6;

};

// ---------------------------------------------------------------------------
// Structure d'un processus - toutes les infos sur un processus
// ---------------------------------------------------------------------------

struct proc {
    enum proc_state state;      // État actuel du processus
    int pid;                    // Numéro d'identification du processus
    struct proc *parent;        // Pointeur vers le processus parent
    void *pagetable;            // Table des pages du processus
    uint64 sz;                  // Taille de la mémoire utilisée
    uint64 kstack;              // Pile du kernel
    struct trapframe *trapframe; // Registres à restaurer après une interruption
    struct context context;     // Contexte du processeur (registres préservés)
    int exit_code;              // Code de sortie quand le processus termine
    char name[32];              // Nom du processus (pour le débogagge)
};

#endif 