// ===========================================================================
// elf.h - Structures pour lire les fichiers binaires ELF
// ===========================================================================

#ifndef KERNEL_PROC_ELF_H
#define KERNEL_PROC_ELF_H

#include "kernel/types.h"

// Code magique pour identifier un fichier ELF
// (les 4 premiers octets d'un fichier ELF valide)
#define ELF_MAGIC 0x464C457FU  // "\x7FELF" en petit-boutiste

// En-tête ELF (format 64 bits)
// Contient les infos sur le fichier et où trouver le reste
struct elfhdr {
    uint32 magic;      // Doit être ELF_MAGIC (0x464C457F)
    uint8  elf[12];    // Classe, format des données, version, OS/ABI...
    uint16 type;       // Type de fichier (exécutable, objet compilé...)
    uint16 machine;    // Architecture (RISC-V = 0xF3)
    uint32 version;    // Numéro de version ELF
    uint64 entry;      // Adresse où commencer l'exécution
    uint64 phoff;      // Où se trouve la table des en-têtes de programme
    uint64 shoff;      // Où se trouve la table des en-têtes de section
    uint32 flags;      // Drapeaux spécifiques à l'architecture
    uint16 ehsize;     // Taille de cet en-tête
    uint16 phentsize;  // Taille d'un seul en-tête de programme
    uint16 phnum;      // Nombre d'en-têtes de programme
    uint16 shentsize;  // Taille d'un seul en-tête de section
    uint16 shnum;      // Nombre d'en-têtes de section
    uint16 shstrndx;   // Index de la table des noms de sections
};

// En-tête de segment du programme
// Décrit une partie du programme à charger en mémoire
struct proghdr {
    uint32 type;       // Type de segment (LOAD = 1 = charger en mémoire)
    uint32 flags;      // Autorisations (lecture, écriture, exécution)
    uint64 off;        // Où dans le fichier ELF se trouve ce segment
    uint64 vaddr;      // Adresse virtuelle où le charger
    uint64 paddr;      // Adresse physique (généralement inutilisée)
    uint64 filesz;     // Taille du segment dans le fichier
    uint64 memsz;      // Taille du segment à laisser en mémoire
    uint64 align;      // Alignement en mémoire
};

// Une valeur pour le champ 'type' d'un segment
#define ELF_PROG_LOAD 1

// Drapeaux pour les permissions d'un segment
#define ELF_PROG_FLAG_X 1  // Peut exécuter (exécutable)
#define ELF_PROG_FLAG_W 2  // Peut écrire
#define ELF_PROG_FLAG_R 4  // Peut lire
