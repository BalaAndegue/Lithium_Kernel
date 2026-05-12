# Rapport Technique – Lithium Kernel : Blocs 1, 2 et 3

**Projet** : Lithium Kernel – Noyau RISC-V minimal de type UNIX  
**Branche** : `gabrielle`  
**Date** : Mai 2026  
**Équipe** : Bala Andegue, Gabrielle Nana, Israel Teme, Tamwo Steve  
**Superviseur** : Professeur Alain Tchana  

---

## Table des matières

1. [Vue d'ensemble du projet](#1-vue-densemble-du-projet)
2. [Bloc 1 – Boot, UART et Console](#2-bloc-1--boot-uart-et-console)
3. [Bloc 2 – Gestion de la mémoire physique et pagination SV39](#3-bloc-2--gestion-de-la-mémoire-physique-et-pagination-sv39)
4. [Bloc 3 – Gestion des processus et ordonnanceur](#4-bloc-3--gestion-des-processus-et-ordonnanceur)
5. [Architecture globale et dépendances](#5-architecture-globale-et-dépendances)
6. [Flux d'initialisation complet du kernel](#6-flux-dinitialisation-complet-du-kernel)
7. [Chaîne de compilation et outils](#7-chaîne-de-compilation-et-outils)
8. [Débogage avec GDB + QEMU](#8-débogage-avec-gdb--qemu)

---

## 1. Vue d'ensemble du projet

Le Lithium Kernel est un noyau pédagogique inspiré d'xv6, ciblant l'architecture **RISC-V 64 bits** et s'exécutant sur l'émulateur **QEMU** (machine `virt`). Il est écrit en **C17** et en **assembleur RISC-V**, compilé avec la toolchain cross-compilée `riscv64-unknown-elf-gcc`.

### Architecture globale du système

```
┌──────────────────────────────────────────────────────────┐
│                    ESPACE UTILISATEUR                    │
│  (Bloc 5-8 : User Space, libc, programmes utilisateur)  │
├──────────────────────────────────────────────────────────┤
│                   APPELS SYSTÈME                         │
│       (Bloc 4 : syscall, trap, interruptions)            │
├───────────────────────┬──────────────────────────────────┤
│   GESTION PROCESSUS   │      GESTION MÉMOIRE             │
│  (Bloc 3 : fork,      │  (Bloc 2 : physmem bitmap,       │
│   scheduler, context) │   pagination SV39, paging)       │
├───────────────────────┴──────────────────────────────────┤
│           DÉMARRAGE + PILOTES SÉRIE                      │
│      (Bloc 1 : entry.S, UART, printf, main.c)            │
├──────────────────────────────────────────────────────────┤
│                    MATÉRIEL RISC-V                        │
│    QEMU virt : CPU RV64IMAC, UART 16550, RAM 128 MB      │
└──────────────────────────────────────────────────────────┘
```

### Carte mémoire physique (QEMU `virt`)

```
0x00000000 ┌──────────────────────────────┐
           │  (espace non mappé / I/O)    │
0x10000000 ├──────────────────────────────┤
           │  UART 16550 (MMIO)           │  ← uart_putchar() écrit ici
0x10000008 ├──────────────────────────────┤
           │  ...                         │
0x80000000 ├──────────────────────────────┤
           │  Code kernel (.text)         │  ← _start chargé ici
           │  Données (.data, .bss)       │
           │  Pile noyau (16 Ko / 4 pages)│
           │  ↓ croît vers le bas         │
           │  [kernel_end = _end]         │
           ├──────────────────────────────┤
           │  Pages libres gérées par     │  ← physmem_alloc_page()
           │  l'allocateur bitmap         │
0x88000000 └──────────────────────────────┘
            (fin des 128 Mo disponibles)
```

---

## 2. Bloc 1 – Boot, UART et Console

### Objectif

Faire démarrer le kernel : configurer le processeur à froid, établir la pile, activer la communication série, et fournir une fonction `printf()` minimale utilisable par tous les blocs suivants.

### Fichiers implémentés

| Fichier | Rôle |
|---------|------|
| `kernel/entry.S` | Point d'entrée assembleur : désactive les interruptions, initialise `sp`, appelle `kernel_main` |
| `kernel/kernel.ld` | Script de l'éditeur de liens : place le kernel à `0x80000000` |
| `kernel/io/uart.c` | Pilote UART 16550 : accès MMIO à l'adresse `0x10000000` |
| `kernel/io/console.c` | Couche d'abstraction `printf()` au-dessus de l'UART |
| `kernel/main.c` | Fonction `kernel_main()` : point de départ du code C |

---

### 2.1 `kernel/entry.S` – Point d'entrée assembleur

C'est la **toute première instruction** exécutée par le CPU après que QEMU a chargé l'ELF en mémoire.

```asm
.section .text
.globl _start

_start:
    # Étape 1 : Désactiver toutes les interruptions
    # mstatus = Machine Status Register
    # Écrire 0 désactive les bits MIE (Machine Interrupt Enable)
    csrw  mstatus, zero

    # Étape 2 : Initialiser le pointeur de pile (Stack Pointer)
    # stack_top est défini dans .bss, 4 pages (16 Ko) réservées
    # La pile RISC-V croît vers le bas : sp pointe vers le HAUT
    la sp, stack_top

    # Étape 3 : Sauter vers le code C
    # call sauvegarde ra (return address) et saute à kernel_main
    call kernel_main

    # Étape 4 : Boucle infinie de sécurité (ne devrait jamais être atteint)
hang:
    wfi          # Wait For Interrupt : réduit la consommation CPU
    j hang

# Section .bss : mémoire non initialisée (mise à zéro par convention)
.section .bss
.align 16        # Alignement sur 16 octets (requis pour la pile)
.space 4096 * 4  # Réservation de 4 pages = 16 Ko pour la pile noyau
stack_top:       # Étiquette : sp pointe ici au démarrage
```

**Pourquoi `csrw mstatus, zero` ?**  
Au démarrage en mode Machine (M-mode), des interruptions parasites pourraient survenir avant que le kernel soit prêt. Écrire 0 dans `mstatus` désactive le bit `MIE` (bit 3) et le bit `MPIE` (bit 7), gelant toute interruption.

**Pourquoi `.align 16` ?**  
La spec ABI RISC-V exige que `sp` soit aligné sur 16 octets lors d'un `call`. Sans ça, les registres flottants (`f0`–`f31`) ne seraient pas sauvegardés correctement sur la pile.

**Pourquoi `wfi` dans la boucle `hang` ?**  
`wfi` (Wait For Interrupt) met le cœur en veille basse consommation jusqu'à la prochaine interruption. Dans QEMU c'est principalement esthétique, mais reproduit le comportement d'un vrai kernel embarqué.

---

### 2.2 `kernel/kernel.ld` – Script de l'éditeur de liens

Le linker script décrit l'organisation mémoire de l'exécutable final.

```ld
OUTPUT_ARCH(riscv)
ENTRY(_start)

BASE_ADDRESS = 0x80000000;  /* Adresse de chargement QEMU virt */

SECTIONS {
    . = BASE_ADDRESS;

    .text   : { *(.text)   }   /* Code exécutable */
    .rodata : { *(.rodata) }   /* Constantes, chaînes littérales */
    .data   : { *(.data)   }   /* Variables globales initialisées */
    .bss    : { *(.bss)    }   /* Variables globales non initialisées (→ 0) */

    _end = .;   /* Symbole exporté : adresse de fin du kernel */
                /* Utilisé par physmem_init() pour savoir où commence la RAM libre */
}
```

**Rôle du symbole `_end`**  
`physmem_init()` fait `extern uint8 _end; uint64 kernel_end = (uint64)&_end;` pour connaître l'adresse juste après le code et les données du kernel. Tout ce qui est au-delà de `_end` est de la RAM utilisable.

**Pourquoi `-z max-page-size=4096` dans `LDFLAGS` ?**  
Sans cette option, GNU ld peut aligner les segments sur 2 Mo (`0x200000`) selon les défauts ELF. On force un alignement à 4 Ko (taille d'une page) pour que les calculs de l'allocateur physique soient cohérents.

---

### 2.3 `kernel/io/uart.c` – Pilote UART 16550

Le UART (Universal Asynchronous Receiver/Transmitter) est le seul canal de communication disponible dès le démarrage, avant toute pagination ou gestion de processus.

```c
/* Adresse MMIO du UART sur QEMU virt */
#define UART_BASE    0x10000000UL
#define UART_TX_REG  0    /* Transmit Holding Register : offset 0 */
#define UART_LSR_REG 5    /* Line Status Register      : offset 5 */
#define UART_LSR_THRE (1 << 5)  /* Transmit Holding Register Empty */

void uart_init(void)
{
    /* QEMU émule un 16550A déjà configuré à 38400 baud, 8N1 */
    /* Aucune configuration nécessaire : le registre de diviseur
       est initialisé par QEMU lui-même                        */
}

void uart_putchar(char c)
{
    /* Attendre que le Transmit Holding Register soit vide */
    while (!(*(volatile uint8*)(UART_BASE + UART_LSR_REG) & UART_LSR_THRE))
        ;  /* Busy-wait : on tourne en boucle jusqu'à pouvoir écrire */

    /* Écrire l'octet dans le registre de transmission */
    *(volatile uint8*)(UART_BASE + UART_TX_REG) = c;
}

void uart_puts(char *s)
{
    while (*s) {
        uart_putchar(*s);
        s++;
    }
}
```

**Mécanisme MMIO (Memory-Mapped I/O)**  
Le UART 16550 est accessible via des adresses mémoire spéciales. En écrivant à `0x10000000`, on envoie un octet au port série. Le mot-clé `volatile` est critique : il empêche le compilateur d'optimiser (mettre en cache ou réordonner) les accès à ces registres.

**Busy-wait vs interruptions**  
À ce stade du kernel, il n'y a pas de gestionnaire d'interruptions. On utilise donc une attente active : on lit le bit `THRE` du `LSR` en boucle jusqu'à ce qu'il soit à 1. C'est inefficace mais fiable pour un kernel de démarrage.

---

### 2.4 `kernel/io/console.c` – `printf()` noyau

Une implémentation minimale de `printf()` en partant de zéro, sans `stdio.h` ni libc.

```c
void printf(char *fmt, ...)
{
    /* Pointeur vers les arguments variables (après fmt sur la pile) */
    char **arg = (char**)(&fmt + 1);

    for (p = fmt; *p; p++) {
        if (*p != '%') {
            uart_putchar(*p);   /* Caractère normal */
            continue;
        }
        p++;  /* Avancer après '%' */

        switch (*p) {
            case 's':  /* %s : chaîne de caractères */
                print_string(*((char**)arg++));
                break;

            case 'd':  /* %d : entier signé décimal */
                print_int(*((int*)arg++));
                break;

            case 'x':  /* %x : entier 64-bit hexadécimal */
                print_hex(*((uint64*)arg++));
                break;

            case 'p':  /* %p : pointeur (= %x avec cast) */
                print_hex((uint64)*arg++);
                break;

            case '%':  /* %% : affiche le caractère '%' */
                uart_putchar('%');
                break;
        }
    }
}
```

**Fonctions internes** :

- `print_int(int n)` : conversion entier → décimal par divisions successives par 10, stockage dans un buffer local, affichage en ordre inverse.
- `print_hex(uint64 n)` : conversion entier → hexadécimal par masquage 4 bits (`n & 0xF`), préfixe `0x`.
- `print_string(char *s)` : délégation à `uart_puts()`.

**Pourquoi pas `va_list` / `va_arg` ?**  
Ces macros nécessitent `<stdarg.h>` qui peut ne pas être disponible dans un cross-compilateur `freestanding`. L'accès direct via `char **arg = (char**)(&fmt + 1)` exploite la convention d'appel RISC-V qui place les arguments consécutivement en mémoire (sur la pile, au-delà du premier argument).

---

### 2.5 `kernel/main.c` – Version Bloc 1

```c
#include "kernel/io/uart.h"
#include "kernel/io/console.h"
#include "kernel/mem/physmem.h"
#include "kernel/mem/paging.h"

void kernel_main(void)
{
    uart_init();
    printf("Lithium Kernel starting...\n");
    printf("========================================\n");

    physmem_init();   /* Ajouté en Bloc 2 */
    paging_init();    /* Ajouté en Bloc 2 */

    /* Tests d'allocation (Bloc 2) */
    uint64 page1 = physmem_alloc_page();
    printf("Allocated page at: %p\n", (void*)page1);
    ...

    while (1);
}
```

À la fin du Bloc 1, `kernel_main` ne fait qu'initialiser l'UART, afficher un message, puis boucler à l'infini. Les lignes mémoire et processus sont ajoutées dans les blocs suivants.

---

### 2.6 Résumé Bloc 1

| Composant | Résultat |
|-----------|----------|
| CPU démarré à `0x80000000` | ✅ |
| Interruptions désactivées | ✅ |
| Pile de 16 Ko initialisée | ✅ |
| UART 16550 opérationnel | ✅ |
| `printf()` avec %s/%d/%x/%p | ✅ |
| Message de boot affiché | ✅ |

**Sortie QEMU attendue (Bloc 1 seul)** :
```
Lithium Kernel starting...
========================================
```

---

## 3. Bloc 2 – Gestion de la mémoire physique et pagination SV39

### Objectif

Mettre en place la couche mémoire complète du kernel :
1. Un **allocateur de pages physiques** basé sur une bitmap
2. Un système de **tables de pages SV39** à 3 niveaux
3. L'**activation de la pagination** via le registre `satp`

### Fichiers implémentés

| Fichier | Rôle |
|---------|------|
| `include/kernel/types.h` | Types de base (`uint8`, `uint64`, etc.) |
| `include/kernel/mem/layout.h` | Constantes de disposition mémoire |
| `include/kernel/mem/physmem.h` | Interface publique de l'allocateur physique |
| `include/kernel/mem/virtual_memory.h` | Macros et types SV39 (bits PTE, VPN) |
| `include/kernel/mem/paging.h` | Interface publique du gestionnaire de pages |
| `kernel/mem/physmem.c` | Implémentation bitmap |
| `kernel/mem/paging.c` | Implémentation tables de pages SV39 |

---

### 3.1 `include/kernel/types.h` – Types fondamentaux

```c
typedef unsigned char   uint8;   /* 1 octet non signé  */
typedef unsigned short  uint16;  /* 2 octets non signés */
typedef unsigned int    uint32;  /* 4 octets non signés */
typedef unsigned long   uint64;  /* 8 octets non signés */

typedef signed char     int8;
typedef signed short    int16;
typedef signed int      int32;
typedef signed long     int64;
```

**Pourquoi ces typedef ?**  
En mode `freestanding` (sans libc), `<stdint.h>` n'est pas garanti. Ces alias assurent des tailles exactes sur RISC-V 64 bits : `unsigned long` fait toujours 8 octets sur RV64.

---

### 3.2 `include/kernel/mem/layout.h` – Carte mémoire

```c
/* Adresse physique de base du kernel (QEMU virt charge ici) */
#define KERNEL_BASE_ADDR  0x80000000UL

/* Taille totale de la RAM disponible : 128 Mo */
#define PHYS_MEM_SIZE     (128 * 1024 * 1024UL)

/* Taille d'une page : 4 Ko = 4096 octets */
#define PAGE_SIZE         4096UL

/* Nombre total de pages dans la RAM */
#define NUM_PAGES         (PHYS_MEM_SIZE / PAGE_SIZE)   /* = 32 768 */

/* Conversions adresse physique <-> virtuelle
   Actuellement en identité (pas d'offset) :
   virt == phys tant qu'on n'a pas de remappage kernel high */
#define phys_to_virt(phys)  ((void *)((uint64)(phys)))
#define virt_to_phys(virt)  ((uint64)(virt))
```

**Pourquoi `UL` sur les constantes ?**  
Sans suffixe, une constante comme `128 * 1024 * 1024` est calculée en `int` 32 bits et déborde (`128 Mo > INT_MAX` sur certaines cibles). `UL` force l'évaluation en `unsigned long` (64 bits sur RV64).

**Pourquoi l'identité `phys == virt` ?**  
En Bloc 2 on fait ce qu'on appelle un *identity mapping* : adresse virtuelle = adresse physique pour tout le kernel. Cela permet d'activer la pagination sans que le code déjà en exécution "saute" vers de mauvaises adresses.

---

### 3.3 `include/kernel/mem/virtual_memory.h` – Structures SV39

L'architecture SV39 de RISC-V définit un espace d'adressage virtuel de **39 bits** avec **3 niveaux** de tables de pages.

#### Format d'une adresse virtuelle SV39

```
63          39 38        30 29        21 20        12 11          0
┌─────────────┬────────────┬────────────┬────────────┬────────────┐
│  Extension  │   VPN[2]   │   VPN[1]   │   VPN[0]   │  Offset   │
│  (signe)    │  (9 bits)  │  (9 bits)  │  (9 bits)  │ (12 bits) │
└─────────────┴────────────┴────────────┴────────────┴────────────┘
                   ↑              ↑              ↑
              Index dans     Index dans     Index dans
              Table Niveau 2  Table Niveau 1  Table Niveau 0
```

#### Format d'une entrée de table de pages (PTE – Page Table Entry)

```
63    54 53      10 9   8  7  6  5  4  3  2  1  0
┌───────┬──────────┬─────┬──┬──┬──┬──┬──┬──┬──┬──┐
│  RSW  │  PPN[2:0]│ RSW │ D│ A│ G│ U│ X│ W│ R│ V│
│ (10b) │ (44 bits)│(2b) │  │  │  │  │  │  │  │  │
└───────┴──────────┴─────┴──┴──┴──┴──┴──┴──┴──┴──┘
                         ↑  ↑  ↑  ↑  ↑  ↑  ↑  ↑
                    Dirty  Accessed Global User Execute Write Read Valid
```

**Définitions des bits de permission** :

```c
typedef uint64 pte_t;

#define PTE_V  (1 << 0)  /* Valid   : l'entrée est valide (doit être à 1) */
#define PTE_R  (1 << 1)  /* Read    : la page peut être lue */
#define PTE_W  (1 << 2)  /* Write   : la page peut être écrite */
#define PTE_X  (1 << 3)  /* Execute : la page contient du code exécutable */
#define PTE_U  (1 << 4)  /* User    : accessible depuis le mode U (utilisateur) */
#define PTE_G  (1 << 5)  /* Global  : présent dans tous les espaces d'adressage */
#define PTE_A  (1 << 6)  /* Accessed: mis à 1 par le HW lors d'un accès */
#define PTE_D  (1 << 7)  /* Dirty   : mis à 1 par le HW lors d'une écriture */

/* Masque pour extraire l'adresse physique (bits [53:10]) */
#define PTE_ADDR_MASK  0x3FFFFFFFFC00UL

/* Extraire le PPN (Physical Page Number) depuis une PTE */
#define PTE_ADDR(pte)  ((pte) & PTE_ADDR_MASK)

/* Construire une PTE à partir d'une adresse physique + flags */
#define MAKE_PTE(phys_addr, flags)  (((phys_addr) & PTE_ADDR_MASK) | (flags))

/* Extraire les Virtual Page Numbers depuis une adresse virtuelle */
#define VPN2(va)  (((va) >> 30) & 0x1FF)  /* Bits 38–30 : index niveau 2 */
#define VPN1(va)  (((va) >> 21) & 0x1FF)  /* Bits 29–21 : index niveau 1 */
#define VPN0(va)  (((va) >> 12) & 0x1FF)  /* Bits 20–12 : index niveau 0 */
```

---

### 3.4 `kernel/mem/physmem.c` – Allocateur bitmap

#### Principe de la bitmap

On gère 32 768 pages (128 Mo / 4 Ko). Chaque page est représentée par **1 bit** dans un tableau :
- Bit à **1** → page **libre**
- Bit à **0** → page **allouée**

```
Bitmap (4 096 octets au total) :

Octet 0  : bits  0– 7  → pages   0–  7
Octet 1  : bits  8–15  → pages   8– 15
...
Octet N  : bits N*8 – N*8+7 → pages N*8 – N*8+7
```

#### Structure de données

```c
static uint8  phys_bitmap[NUM_PAGES / 8];  /* 32768 / 8 = 4096 octets */
static uint64 free_pages_count;            /* Compteur de pages libres */
```

#### Initialisation

```c
void physmem_init(void)
{
    extern uint8 _end;                    /* Symbole linker : fin du kernel */
    uint64 kernel_end = (uint64)&_end;

    /* 1) Marquer TOUTES les pages comme libres */
    for (uint64 i = 0; i < NUM_PAGES / 8; i++)
        phys_bitmap[i] = 0xFF;            /* 0xFF = tous les bits à 1 = libres */
    free_pages_count = NUM_PAGES;

    /* 2) Calculer l'index de la première page après le kernel */
    /*    Arrondir _end au prochain multiple de PAGE_SIZE (alignement haut) */
    uint64 first_free_page = (kernel_end + PAGE_SIZE - 1) / PAGE_SIZE;

    /* 3) Marquer les pages occupées par le kernel comme allouées */
    for (uint64 i = 0; i < first_free_page; i++)
        if (is_page_free(i))
            mark_page_allocated(i);

    printf("physmem_init: %d pages total, %d pages free\n",
           NUM_PAGES, free_pages_count);
}
```

**Exemple concret** : si `_end = 0x800E0000` (kernel de ~900 Ko), alors `first_free_page = 0x800E0000 / 0x1000 = 0x800E0 = 524 512 / 4096`... En réalité on divise par `PAGE_SIZE` sans la base, et `_end` est une adresse absolue. Les premières `first_free_page` pages (depuis la page 0) sont réservées au kernel.

#### Opérations sur la bitmap

```c
/* Vérifier si la page d'index i est libre (retourne 1 si libre) */
static int is_page_free(uint64 page_index)
{
    uint64 byte_index = page_index / 8;
    uint64 bit_index  = page_index % 8;
    return (phys_bitmap[byte_index] >> bit_index) & 1;
}

/* Marquer la page d'index i comme libre (bit → 1) */
static void mark_page_free(uint64 page_index)
{
    phys_bitmap[page_index / 8] |=  (1 << (page_index % 8));
    free_pages_count++;
}

/* Marquer la page d'index i comme allouée (bit → 0) */
static void mark_page_allocated(uint64 page_index)
{
    phys_bitmap[page_index / 8] &= ~(1 << (page_index % 8));
    free_pages_count--;
}
```

**Illustration bit-à-bit** pour la page 5 :

```
page_index = 5
byte_index = 5 / 8 = 0      → octet 0 de la bitmap
bit_index  = 5 % 8 = 5      → bit 5 de cet octet

Bitmap[0] avant : 1111 1111
(1 << 5)        : 0010 0000
~(1 << 5)       : 1101 1111
Bitmap[0] après : 1101 1111  ← bit 5 à 0 = page 5 allouée
```

#### Allocation d'une page

```c
uint64 physmem_alloc_page(void)
{
    for (uint64 i = 0; i < NUM_PAGES; i++) {
        if (is_page_free(i)) {
            mark_page_allocated(i);
            return i * PAGE_SIZE;   /* Adresse physique = index × 4096 */
        }
    }
    printf("physmem_alloc_page: out of memory!\n");
    return 0;   /* 0 = erreur (jamais une adresse valide allouable) */
}
```

**Complexité** : O(n) dans le pire cas (n = 32 768). Acceptable pour un noyau pédagogique ; un vrai noyau utiliserait un **buddy allocator** ou une **free list** pour O(1).

#### Libération d'une page

```c
void free_physical_page(uint64 phys_addr)
{
    uint64 page_index = phys_addr / PAGE_SIZE;

    /* Validation 1 : l'adresse doit être alignée sur 4 Ko */
    if (phys_addr % PAGE_SIZE != 0) {
        printf("free_physical_page: address non alignée\n");
        return;
    }

    /* Validation 2 : l'index doit être dans la plage valide */
    if (page_index >= NUM_PAGES) {
        printf("free_physical_page: address hors plage\n");
        return;
    }

    /* Validation 3 : ne pas libérer une page déjà libre (double-free) */
    if (!is_page_free(page_index))
        mark_page_free(page_index);
}
```

**Les trois validations protègent contre** :
1. Un pointeur mal aligné (bug dans l'appelant)
2. Un pointeur hors de la RAM gérée
3. Un double-free (libérer deux fois la même page corromprait le compteur)

---

### 3.5 `kernel/mem/paging.c` – Tables de pages SV39

#### Vue d'ensemble de la hiérarchie SV39

```
Registre SATP (Supervisor Address Translation and Protection)
   ├── Mode = 8 (SV39)
   └── PPN → Table Niveau 2 (root, 512 entrées × 8 octets = 4 Ko)
               ├── PTE[VPN2] → Table Niveau 1 (512 entrées)
               │                  ├── PTE[VPN1] → Table Niveau 0 (512 entrées)
               │                  │                  └── PTE[VPN0] → Page physique
               │                  └── ...
               └── ...

Chaque niveau traduit 9 bits. L'offset de page traduit les 12 bits restants.
Espace d'adressage virtuel max : 2^39 = 512 Go
```

#### Création d'une table de pages

```c
void* create_page_table(void)
{
    uint64 phys = physmem_alloc_page();   /* Allouer 1 page physique (4 Ko) */
    if (phys == 0) return NULL;

    uint64 *table = (uint64*)phys_to_virt(phys);

    /* Initialiser les 512 entrées à 0 = toutes invalides (PTE_V = 0) */
    for (int i = 0; i < 512; i++)
        table[i] = 0;

    return table;
}
```

Une table de pages occupe exactement **une page physique** : 512 entrées × 8 octets = 4 096 octets = 1 page.

#### La fonction `walk()` – traversée des 3 niveaux

C'est le cœur du système de pagination. Elle prend une adresse virtuelle et retourne un **pointeur vers la PTE finale** (niveau 0), en créant les niveaux intermédiaires si `alloc=1`.

```c
static uint64* walk(void *pagetable, uint64 virt_addr, int alloc)
{
    uint64 vpn2 = VPN2(virt_addr);   /* Bits 38–30 */
    uint64 vpn1 = VPN1(virt_addr);   /* Bits 29–21 */
    uint64 vpn0 = VPN0(virt_addr);   /* Bits 20–12 */

    /* ── NIVEAU 2 ── */
    uint64 *pte_l2 = &((uint64*)pagetable)[vpn2];

    if (!(*pte_l2 & PTE_V)) {           /* Entrée invalide ? */
        if (!alloc) return NULL;
        uint64 phys = physmem_alloc_page();
        if (phys == 0) return NULL;
        *pte_l2 = MAKE_PTE(phys, PTE_V);           /* Pointer vers niveau 1 */
        /* Initialiser les 512 PTEs du niveau 1 à 0 */
        uint64 *l1 = (uint64*)phys_to_virt(phys);
        for (int i = 0; i < 512; i++) l1[i] = 0;
    }

    /* ── NIVEAU 1 ── */
    uint64 *level1 = (uint64*)phys_to_virt(PTE_ADDR(*pte_l2));
    uint64 *pte_l1 = &level1[vpn1];

    if (!(*pte_l1 & PTE_V)) {
        if (!alloc) return NULL;
        uint64 phys = physmem_alloc_page();
        if (phys == 0) return NULL;
        *pte_l1 = MAKE_PTE(phys, PTE_V);
        uint64 *l0 = (uint64*)phys_to_virt(phys);
        for (int i = 0; i < 512; i++) l0[i] = 0;
    }

    /* ── NIVEAU 0 ── : retourner le pointeur vers la PTE finale */
    uint64 *level0 = (uint64*)phys_to_virt(PTE_ADDR(*pte_l1));
    return &level0[vpn0];
}
```

**Exemple de traversée pour l'adresse `0x80002000`** :

```
0x80002000 en binaire (39 bits significatifs) :
  VPN2 = (0x80002000 >> 30) & 0x1FF = 0x200 & 0x1FF = 0   (bits 38–30)
  VPN1 = (0x80002000 >> 21) & 0x1FF = 0x400 & 0x1FF = 0   (bits 29–21)
  VPN0 = (0x80002000 >> 12) & 0x1FF = 0x80002 & 0x1FF = 2 (bits 20–12)
  Offset = 0x000                                            (bits 11–0)

Traversée :
  1. root_table[0] → (créer si absent) → table_l1
  2. table_l1[0]   → (créer si absent) → table_l0
  3. table_l0[2]   → ← PTE finale
```

#### Mappage d'une page virtuelle → physique

```c
int map_page(void *pagetable, uint64 virt_addr, uint64 phys_addr, uint64 flags)
{
    uint64 *pte = walk(pagetable, virt_addr, 1);   /* alloc=1 : créer les niveaux */
    if (pte == NULL) return -1;
    if (*pte & PTE_V) return -1;                   /* Déjà mappée : erreur */

    *pte = MAKE_PTE(phys_addr, flags | PTE_V);     /* Écrire la PTE finale */
    return 0;
}
```

**Combinaisons de flags typiques** :

| Usage | Flags |
|-------|-------|
| Code kernel | `PTE_R \| PTE_W \| PTE_X` |
| Données kernel | `PTE_R \| PTE_W` |
| Code utilisateur | `PTE_R \| PTE_X \| PTE_U` |
| Données utilisateur | `PTE_R \| PTE_W \| PTE_U` |
| Guard page (protection stack) | `0` (invalide) |

#### Activation de la pagination via le registre `satp`

```c
void switch_page_table(void *pagetable)
{
    uint64 phys = virt_to_phys(pagetable);

    /*
     * Registre SATP (64 bits) :
     *   [63:60] = Mode   : 8 = SV39
     *   [59:44] = ASID   : Address Space ID (0 pour le kernel)
     *   [43:0]  = PPN    : Physical Page Number de la table root
     *                      PPN = adresse_physique >> 12
     */
    uint64 satp = (8UL << 60) | ((phys >> 12) & 0x0FFFFFFFFFFFFFUL);

    asm volatile("csrw satp, %0" : : "r"(satp));   /* Activer la pagination */
    asm volatile("sfence.vma zero, zero");           /* Invalider tout le TLB */
}
```

**Pourquoi `sfence.vma` après `csrw satp` ?**  
Le TLB (Translation Lookaside Buffer) est un cache matériel des traductions d'adresses. Après avoir changé la table des pages, les entrées TLB anciennes pointeraient vers de mauvaises tables. `sfence.vma` les invalide toutes, forçant le CPU à relire les tables en mémoire lors du prochain accès.

#### Initialisation du kernel (identity mapping 128 Mo)

```c
void paging_init(void)
{
    printf("paging_init: initializing SV39 page tables\n");

    kernel_pagetable = create_page_table();   /* Table root du kernel */

    /* Identity map : mapper virtuellement chaque adresse sur elle-même
       pour tout l'espace physique de 128 Mo                          */
    for (uint64 i = 0; i < PHYS_MEM_SIZE; i += PAGE_SIZE) {
        if (map_page(kernel_pagetable, i, i, PTE_R | PTE_W | PTE_X) != 0) {
            printf("paging_init: failed to map page %p\n", i);
            return;
        }
    }

    switch_page_table(kernel_pagetable);   /* Activer via SATP */
    printf("paging_init: kernel page table activated\n");
}
```

**Coût de l'identity mapping** :  
Mapper 128 Mo avec des pages de 4 Ko = 32 768 appels à `map_page()`. Chaque appel peut allouer jusqu'à 2 pages intermédiaires (niveaux 1 et 2). Dans la pratique, les pages de même VPN2 et VPN1 partagent les niveaux intermédiaires, donc l'empreinte réelle est beaucoup plus faible.

---

### 3.6 Résumé Bloc 2

| Composant | Résultat |
|-----------|----------|
| Bitmap 32 768 bits pour 128 Mo | ✅ |
| `physmem_alloc_page()` | ✅ |
| `free_physical_page()` avec validations | ✅ |
| Tables SV39 à 3 niveaux | ✅ |
| `map_page()` / `unmap_page()` | ✅ |
| Identity mapping 128 Mo | ✅ |
| Pagination activée via `satp` | ✅ |

**Sortie QEMU attendue (Bloc 1 + Bloc 2)** :
```
Lithium Kernel starting...
========================================
physmem_init: 32768 pages total, 32544 pages free
paging_init: initializing SV39 page tables
paging_init: kernel page table activated
Allocated page at: 0x80054000
Allocated page at: 0x80055000
Freed page at: 0x80054000
Free pages: 32543
Memory management initialized successfully!
```

---

## 4. Bloc 3 – Gestion des processus et ordonnanceur

### Objectif

Implémenter la gestion complète du cycle de vie des processus UNIX :
1. **Structure de données** du processus (`struct proc`)
2. **Allocation/libération** de processus (`alloc_proc`, `free_proc`)
3. **Contrôle de processus** : `fork()`, `exit()`, `wait()`, `sleep()`, `wakeup()`
4. **Ordonnanceur round-robin** (`scheduler`, `yield`)
5. **Changement de contexte** en assembleur (`context_switch`)

### Fichiers implémentés

| Fichier | Rôle |
|---------|------|
| `include/kernel/proc/process.h` | `struct proc`, `struct context`, `struct trapframe`, `enum proc_state` |
| `include/kernel/proc/globals.h` | `proc_table[]`, `current_process`, `NPROC` |
| `include/kernel/proc/control.h` | Interface : `fork`, `exit`, `wait`, `sleep`, `wakeup`, `alloc_proc`, `free_proc` |
| `include/kernel/proc/scheduler.h` | Interface : `scheduler`, `yield`, `context_switch` |
| `kernel/proc/process.c` | `alloc_proc`, `free_proc`, `proc_init`, `myproc` |
| `kernel/proc/control.c` | `fork`, `exit`, `wait`, `sleep`, `wakeup` |
| `kernel/proc/scheduler.c` | `scheduler` (round-robin), `yield` |
| `kernel/proc/switch_context.S` | `context_switch` en assembleur RISC-V |

---

### 4.1 `include/kernel/proc/process.h` – Structures fondamentales

#### États d'un processus

```c
enum proc_state {
    PROC_UNUSED   = 0,  /* Case libre dans proc_table, aucune ressource allouée  */
    PROC_USED,          /* Structure allouée mais pas encore exécutable           */
    PROC_SLEEPING,      /* En attente d'un événement (sleep/wakeup)              */
    PROC_RUNNABLE,      /* Prêt à être choisi par le scheduler                   */
    PROC_RUNNING,       /* Actuellement en train de s'exécuter sur le CPU        */
    PROC_ZOMBIE         /* Terminé (exit() appelé), attend que le parent fasse wait() */
};
```

**Cycle de vie d'un processus** :

```
        alloc_proc()       proc_init() / fork()
UNUSED ─────────────→ USED ────────────────────→ RUNNABLE
                                                     │
                              ┌──────────────────────┘
                              │ scheduler() choisit
                              ↓
                          RUNNING
                         /       \
              yield() ou          exit()
              sleep()              │
                │                  ↓
                ↓               ZOMBIE ──── wait() ──→ UNUSED
            SLEEPING                         (parent)
                │
           wakeup()
                │
                ↓
           RUNNABLE
```

#### Contexte CPU (`struct context`)

Lors d'un changement de contexte, le kernel sauvegarde uniquement les **registres preserved** (callee-saved) selon la convention d'appel RISC-V :

```c
struct context {
    uint64 ra;   /* x1  : Return Address (adresse de retour dans context_switch) */
    uint64 sp;   /* x2  : Stack Pointer (pile noyau du processus)                */
    uint64 s0;   /* x8  : Saved register 0  ┐                                   */
    uint64 s1;   /* x9  : Saved register 1  │                                   */
    uint64 s2;   /* x18 : Saved register 2  │  Ces registres doivent être       */
    uint64 s3;   /* x19 : Saved register 3  │  préservés par la callee          */
    uint64 s4;   /* x20 : Saved register 4  │  (convention RISC-V ABI)          */
    uint64 s5;   /* x21 : Saved register 5  │                                   */
    uint64 s6;   /* x22 : Saved register 6  │                                   */
    uint64 s7;   /* x23 : Saved register 7  │                                   */
    uint64 s8;   /* x24 : Saved register 8  │                                   */
    uint64 s9;   /* x25 : Saved register 9  │                                   */
    uint64 s10;  /* x26 : Saved register 10 │                                   */
    uint64 s11;  /* x27 : Saved register 11 ┘                                   */
};
```

**Pourquoi seulement ces registres ?**  
Les registres `a0`–`a7` (arguments), `t0`–`t6` (temporaires) sont caller-saved : la fonction appelante est responsable de les sauvegarder si elle en a besoin après l'appel. En revanche `s0`–`s11`, `sp` et `ra` sont callee-saved : `context_switch` les sauvegarde car le scheduler reprendra l'exécution au retour de cette fonction.

#### Trapframe (`struct trapframe`)

La trapframe sauvegarde **tous** les registres utilisateur lors d'une exception ou d'un appel système (prêt pour Bloc 4) :

```c
struct trapframe {
    /* Informations kernel nécessaires pour le retour au mode superviseur */
    uint64 kernel_satp;    /* Valeur de satp du kernel (table de pages kernel) */
    uint64 kernel_sp;      /* Pile noyau du processus (kstack)                 */
    uint64 kernel_trap;    /* Adresse du gestionnaire de trap                  */
    uint64 epc;            /* Valeur de sepc : adresse qui a déclenché le trap */
    uint64 kernel_hartid;  /* ID du cœur CPU (pour SMP futur)                  */

    /* Tous les registres généraux x0–x31 */
    uint64 ra;   /* x1  */   uint64 sp;   /* x2  */
    uint64 gp;   /* x3  */   uint64 tp;   /* x4  */
    uint64 t0;   /* x5  */   uint64 t1;   /* x6  */   uint64 t2;  /* x7  */
    uint64 s0;   /* x8  */   uint64 s1;   /* x9  */
    uint64 a0;   /* x10 */   uint64 a1;   /* x11 */   uint64 a2;  /* x12 */
    uint64 a3;   /* x13 */   uint64 a4;   /* x14 */   uint64 a5;  /* x15 */
    uint64 a6;   /* x16 */   uint64 a7;   /* x17 */   /* a7 = numéro syscall */
    uint64 s2;   /* x18 */ ... uint64 s11; /* x27 */
    uint64 t3;   /* x28 */ ... uint64 t6;  /* x31 */
};
```

**Rôle de `a0` dans le trapframe** : lors d'un `fork()`, `a0` est mis à `0` pour l'enfant (car `fork()` retourne 0 dans le processus enfant et le PID de l'enfant dans le parent).

#### Structure d'un processus (`struct proc`)

```c
struct proc {
    enum proc_state    state;      /* État courant (UNUSED, RUNNABLE, etc.)         */
    int                pid;        /* Process ID unique (commence à 1)              */
    struct proc       *parent;     /* Pointeur vers le processus parent (NULL=init) */
    void              *pagetable;  /* Table de pages virtuelle du processus         */
    uint64             sz;         /* Taille de la mémoire virtuelle utilisée       */
    uint64             kstack;     /* Adresse du haut de la pile noyau              */
    struct trapframe  *trapframe;  /* Registres sauvegardés lors des traps          */
    struct context     context;    /* Contexte CPU pour context_switch()            */
    int                exit_code;  /* Code de sortie (renvoyé à wait() du parent)  */
    char               name[32];   /* Nom lisible du processus (débogage)           */
};
```

---

### 4.2 `include/kernel/proc/globals.h` – Variables globales partagées

```c
#define NPROC 64   /* Nombre maximum de processus simultanés */

extern struct proc  proc_table[NPROC];    /* Table globale de TOUS les processus */
extern struct proc *current_process;      /* Processus actuellement en cours     */
extern int          next_pid;             /* Prochain PID à attribuer (croissant) */
```

**Pourquoi `NPROC = 64` ?**  
C'est une limite statique pour éviter toute allocation dynamique dans le kernel. Tous les processus sont pré-alloués dans un tableau contigu. Pour un vrai noyau, on utiliserait une liste chaînée avec allocation dynamique.

---

### 4.3 `kernel/proc/process.c` – Gestion du cycle de vie

#### `alloc_proc()` – Allouer un nouveau processus

```c
struct proc* alloc_proc(void)
{
    /* Parcourir la table pour trouver une case UNUSED */
    for (int i = 0; i < NPROC; i++) {
        if (proc_table[i].state == PROC_UNUSED) {
            struct proc *p = &proc_table[i];

            /* Initialiser les champs de base */
            p->state     = PROC_USED;      /* Marquée, pas encore prête */
            p->pid       = next_pid++;     /* PID unique, incrémental  */
            p->parent    = NULL;
            p->pagetable = NULL;
            p->sz        = 0;
            p->kstack    = 0;
            p->trapframe = NULL;
            p->exit_code = 0;
            p->name[0]   = '\0';

            return p;
        }
    }
    printf("alloc_proc: no more processes!\n");
    return NULL;
}
```

#### `free_proc()` – Libérer toutes les ressources

```c
void free_proc(struct proc *p)
{
    if (p == NULL) return;

    /* 1. Libérer la table des pages virtuelle */
    if (p->pagetable != NULL) {
        free_page_table(p->pagetable);
        p->pagetable = NULL;
    }

    /* 2. Libérer la pile noyau (kstack) */
    if (p->kstack != 0) {
        /* kstack pointe vers le HAUT de la pile,
           la page commence PAGE_SIZE octets plus bas */
        uint64 kstack_page = p->kstack - PAGE_SIZE;
        free_physical_page(virt_to_phys((void*)kstack_page));
        p->kstack = 0;
    }

    /* 3. Libérer la trapframe */
    if (p->trapframe != NULL) {
        free_physical_page(virt_to_phys(p->trapframe));
        p->trapframe = NULL;
    }

    /* 4. Remettre la case comme libre */
    p->state = PROC_UNUSED;
}
```

#### `proc_init()` – Créer le processus `init` (PID 1)

```c
void proc_init(void)
{
    printf("proc_init: initializing process subsystem\n");

    /* Marquer toutes les cases comme libres */
    for (int i = 0; i < NPROC; i++)
        proc_table[i].state = PROC_UNUSED;

    /* Créer le premier processus : init (PID 1) */
    struct proc *init = alloc_proc();
    if (init == NULL) { printf("proc_init: failed!\n"); return; }

    /* Allouer sa pile noyau (1 page = 4 Ko) */
    uint64 kstack_page = physmem_alloc_page();
    /* kstack pointe vers le HAUT (la pile croît vers le bas) */
    init->kstack = (uint64)phys_to_virt(kstack_page) + PAGE_SIZE;

    /* Allouer la trapframe (1 page) */
    uint64 tf_page = physmem_alloc_page();
    init->trapframe = (struct trapframe*)phys_to_virt(tf_page);

    /* Créer sa table de pages virtuelle */
    init->pagetable = create_page_table();

    /* Marquer comme prêt à tourner */
    init->state = PROC_RUNNABLE;
    init->name[0] = 'i'; init->name[1] = 'n';
    init->name[2] = 'i'; init->name[3] = 't'; init->name[4] = '\0';

    /* C'est le processus courant dès la fin de l'init */
    current_process = init;

    printf("proc_init: init process PID=%d\n", init->pid);
}
```

---

### 4.4 `kernel/proc/control.c` – Appels de contrôle UNIX

#### `fork()` – Créer un processus enfant

```c
int fork(void)
{
    struct proc *parent = current_process;
    if (parent == NULL) return -1;

    /* 1. Allouer la structure de l'enfant */
    struct proc *child = alloc_proc();
    if (child == NULL) return -1;

    /* 2. Hériter des attributs du parent */
    child->parent = parent;
    child->sz     = parent->sz;

    /* 3. Allouer la pile noyau de l'enfant */
    uint64 kstack_page = physmem_alloc_page();
    if (kstack_page == 0) { free_proc(child); return -1; }
    child->kstack = (uint64)phys_to_virt(kstack_page) + PAGE_SIZE;

    /* 4. Allouer et copier la trapframe */
    uint64 tf_page = physmem_alloc_page();
    if (tf_page == 0) { free_proc(child); return -1; }
    child->trapframe = (struct trapframe*)phys_to_virt(tf_page);

    if (parent->trapframe != NULL) {
        *child->trapframe = *parent->trapframe;  /* Copie complète des registres */
        child->trapframe->a0 = 0;                /* L'enfant retourne 0 de fork() */
    }

    /* 5. Créer une nouvelle table de pages (COW serait ici en version avancée) */
    child->pagetable = create_page_table();
    if (child->pagetable == NULL) { free_proc(child); return -1; }

    /* 6. Rendre l'enfant exécutable */
    child->state = PROC_RUNNABLE;

    printf("fork: processus enfant %d créé depuis parent %d\n",
           child->pid, parent->pid);

    /* Retourner le PID de l'enfant au parent */
    return child->pid;
}
```

**Sémantique de `fork()`** :
- Dans le **parent** : retourne le PID de l'enfant (> 0)
- Dans l'**enfant** : retourne 0 (grâce à `child->trapframe->a0 = 0`)
- En cas d'erreur : retourne -1

**Ce qui n'est PAS encore fait** (pour Bloc 4+) :
- Copie de l'espace mémoire utilisateur (Copy-on-Write ou full copy)
- Les mappages de la table de pages du parent ne sont pas dupliqués

#### `exit()` – Terminer le processus courant

```c
void exit(int status)
{
    struct proc *p = current_process;
    if (p == NULL) return;

    printf("exit: process %d exited with status %d\n", p->pid, status);

    /* 1. Sauvegarder le code de sortie pour wait() */
    p->exit_code = status;

    /* 2. Devenir zombie : les ressources ne sont pas libérées ici
       car le parent doit pouvoir lire exit_code via wait() */
    p->state = PROC_ZOMBIE;

    /* 3. Réveiller le parent s'il attendait dans wait() */
    if (p->parent != NULL)
        wakeup(p->parent);

    /* 4. Boucle infinie : on n'a plus de stack disponible
       L'ordonnanceur ne choisira plus jamais un ZOMBIE */
    while (1) asm volatile("wfi");
}
```

**Pourquoi ne pas libérer les ressources directement dans `exit()` ?**  
Parce qu'on est encore en train de s'exécuter sur la pile noyau du processus ! Libérer `kstack` ici causerait un accès mémoire invalide. C'est le rôle de `wait()` dans le parent de finaliser la libération.

#### `wait()` – Attendre un enfant

```c
int wait(uint64 status_addr)
{
    for (int i = 0; i < NPROC; i++) {
        struct proc *child = &proc_table[i];

        /* Chercher un enfant zombie du processus courant */
        if (child->parent == current_process && child->state == PROC_ZOMBIE) {
            int pid = child->pid;

            /* Copier le code de sortie à l'adresse fournie */
            *(int*)status_addr = child->exit_code;

            /* Libérer toutes les ressources de l'enfant */
            free_proc(child);

            return pid;   /* Retourner le PID de l'enfant terminé */
        }
    }
    return -1;   /* Pas d'enfant zombie pour l'instant */
}
```

**Limitation actuelle** : `wait()` ne bloque pas si aucun enfant n'est zombie (retourne -1 immédiatement). La version complète (Bloc 4) utilisera `sleep()` pour attendre.

#### `sleep()` – Suspendre le processus courant

```c
void sleep(void *chan)
{
    struct proc *p = current_process;
    if (p == NULL) return;

    /* chan : canal d'attente (adresse utilisée comme identifiant d'événement) */
    /* Non utilisé actuellement, prévu pour Bloc 4 */
    (void)chan;

    /* Marquer le processus comme dormant */
    p->state = PROC_SLEEPING;

    /* Appeler l'ordonnanceur : il choisira un autre processus */
    scheduler();
}
```

**Le concept de canal d'attente (`chan`)** :  
Dans xv6, `sleep(chan)` et `wakeup(chan)` utilisent une adresse mémoire comme identifiant d'événement. Un processus qui attend la fin d'une E/S dort sur `&inode_lock`. Quand le pilote termine, il appelle `wakeup(&inode_lock)`, réveillant exactement les bons processus. Cette mécanique est prévue pour Bloc 4.

#### `wakeup()` – Réveiller les processus endormis

```c
void wakeup(void *chan)
{
    /* Version simplifiée : réveiller TOUS les processus dormants */
    /* Version Bloc 4 : filtrer sur chan */
    (void)chan;

    for (int i = 0; i < NPROC; i++) {
        if (proc_table[i].state == PROC_SLEEPING) {
            proc_table[i].state = PROC_RUNNABLE;
        }
    }
}
```

---

### 4.5 `kernel/proc/scheduler.c` – Ordonnanceur round-robin

#### `yield()` – Céder volontairement le CPU

```c
void yield(void)
{
    /* Remettre le processus courant comme prêt (il repassera plus tard) */
    if (current_process != NULL && current_process->state == PROC_RUNNING)
        current_process->state = PROC_RUNNABLE;

    /* Déclencher l'ordonnanceur */
    scheduler();
}
```

#### `scheduler()` – Boucle principale de l'ordonnanceur

```c
void scheduler(void)
{
    static int current_index = 0;   /* Mémoriser la position dans la table */

    while (1) {
        for (int i = 0; i < NPROC; i++) {
            struct proc *p = &proc_table[current_index];
            current_index = (current_index + 1) % NPROC;   /* Round-robin */

            if (p->state == PROC_RUNNABLE) {
                struct proc *prev = current_process;

                /* Mettre à jour le processus courant */
                current_process = p;
                p->state = PROC_RUNNING;

                /* Charger la table de pages du processus */
                if (p->pagetable != NULL)
                    switch_page_table(p->pagetable);

                /* Sauvegarder le contexte de prev et restaurer celui de p */
                if (prev != NULL)
                    context_switch(&prev->context, &p->context);
                else
                    context_switch(NULL, &p->context);
            }
        }
    }
}
```

**Algorithme round-robin** :  
L'ordonnanceur parcourt la `proc_table` en anneau, à partir de `current_index`. Il choisit le premier processus `PROC_RUNNABLE` rencontré, effectue le changement de contexte, et reprend la boucle au retour. Chaque processus obtient le CPU jusqu'à ce qu'il appelle `yield()`, `sleep()`, ou `exit()`.

**Visualisation du round-robin** :

```
proc_table : [UNUSED][RUNNING:PID1][RUNNABLE:PID2][ZOMBIE:PID3][RUNNABLE:PID4]...
                          ↑
                    current_process

Après yield() de PID1 :
proc_table : [UNUSED][RUNNABLE:PID1][RUNNING:PID2][ZOMBIE:PID3][RUNNABLE:PID4]...
                                          ↑
                                   current_process
```

---

### 4.6 `kernel/proc/switch_context.S` – Changement de contexte en assembleur

C'est la fonction la plus délicate du bloc 3. Elle doit être en assembleur car elle manipule directement les registres du CPU sans les cacher dans des variables C.

```asm
# void context_switch(struct context *old, struct context *new)
#   a0 = pointeur vers le contexte à SAUVEGARDER (processus actuel)
#   a1 = pointeur vers le contexte à RESTAURER  (nouveau processus)

.section .text
.globl context_switch

context_switch:

    # ═══════ SAUVEGARDER le contexte OLD (a0) ═══════
    # Les offsets correspondent aux champs de struct context dans l'ordre :
    # { ra, sp, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11 }

    sd ra,   0(a0)    # Sauvegarder ra  (return address)
    sd sp,   8(a0)    # Sauvegarder sp  (stack pointer)
    sd s0,  16(a0)    # Sauvegarder s0  ┐
    sd s1,  24(a0)    # Sauvegarder s1  │
    sd s2,  32(a0)    # Sauvegarder s2  │
    sd s3,  40(a0)    # Sauvegarder s3  │  Registres callee-saved
    sd s4,  48(a0)    # Sauvegarder s4  │
    sd s5,  56(a0)    # Sauvegarder s5  │
    sd s6,  64(a0)    # Sauvegarder s6  │
    sd s7,  72(a0)    # Sauvegarder s7  │
    sd s8,  80(a0)    # Sauvegarder s8  │
    sd s9,  88(a0)    # Sauvegarder s9  │
    sd s10, 96(a0)    # Sauvegarder s10 │
    sd s11,104(a0)    # Sauvegarder s11 ┘

    # ═══════ RESTAURER le contexte NEW (a1) ═══════

    ld ra,   0(a1)    # Restaurer ra  → le CPU retournera à l'adresse sauvegardée
    ld sp,   8(a1)    # Restaurer sp  → on bascule sur la PILE du nouveau processus
    ld s0,  16(a1)
    ld s1,  24(a1)
    ld s2,  32(a1)
    ld s3,  40(a1)
    ld s4,  48(a1)
    ld s5,  56(a1)
    ld s6,  64(a1)
    ld s7,  72(a1)
    ld s8,  80(a1)
    ld s9,  88(a1)
    ld s10, 96(a1)
    ld s11,104(a1)

    # ret = jalr x0, ra, 0 → sauter à l'adresse contenue dans ra
    # C'est ra du NOUVEAU processus qui vient d'être chargé !
    # L'exécution reprend là où le nouveau processus a appelé context_switch()
    ret
```

**Ce qui se passe exactement au niveau du CPU** :

```
Avant context_switch(&proc_A->context, &proc_B->context) :
  - CPU exécute proc_A
  - ra = adresse de retour dans scheduler() (pour proc_A)
  - sp = pile noyau de proc_A

Pendant context_switch() :
  - Sauvegarde ra, sp, s0-s11 dans proc_A->context
  - Charge ra, sp, s0-s11 depuis proc_B->context
  - sp est maintenant la pile noyau de proc_B !

Après ret :
  - CPU saute à l'adresse ra de proc_B
  - C'est l'adresse dans scheduler() là où proc_B avait été suspendu
  - L'exécution reprend pour proc_B comme si rien ne s'était passé
```

**Pourquoi `sd` (Store Doubleword) et `ld` (Load Doubleword) ?**  
`sd` et `ld` sont les instructions 64 bits de RISC-V (vs `sw`/`lw` pour 32 bits). Tous nos registres font 64 bits (`uint64`), donc on utilise les variantes 64 bits.

---

### 4.7 Résumé Bloc 3

| Composant | Résultat |
|-----------|----------|
| `struct proc` avec tous les champs | ✅ |
| `struct context` (14 registres callee-saved) | ✅ |
| `struct trapframe` (tous les registres) | ✅ |
| Automate d'états (6 états) | ✅ |
| `alloc_proc()` / `free_proc()` | ✅ |
| `proc_init()` → processus init PID=1 | ✅ |
| `fork()` avec copie trapframe | ✅ |
| `exit()` → état ZOMBIE | ✅ |
| `wait()` → récupère zombie et libère | ✅ |
| `sleep()` / `wakeup()` | ✅ |
| Ordonnanceur round-robin | ✅ |
| `context_switch` RISC-V assembleur | ✅ |

**Sortie QEMU attendue (Bloc 1 + 2 + 3)** :
```
Lithium Kernel starting...
========================================
physmem_init: 32768 pages total, 32544 pages free
paging_init: initializing SV39 page tables
paging_init: kernel page table activated
proc_init: initializing process subsystem
proc_init: init process PID=1
scheduler: entering main loop
```

---

## 5. Architecture globale et dépendances

### Graphe de dépendances entre modules

```
kernel/entry.S
    └─→ kernel/main.c (kernel_main)
            ├─→ kernel/io/uart.c
            │       └─→ include/kernel/io/uart_defs.h
            ├─→ kernel/io/console.c
            │       └─→ kernel/io/uart.c
            ├─→ kernel/mem/physmem.c
            │       ├─→ include/kernel/mem/layout.h
            │       └─→ include/kernel/types.h
            ├─→ kernel/mem/paging.c
            │       ├─→ kernel/mem/physmem.c
            │       ├─→ include/kernel/mem/virtual_memory.h
            │       └─→ include/kernel/mem/layout.h
            └─→ kernel/proc/process.c
                    ├─→ kernel/mem/physmem.c
                    ├─→ kernel/mem/paging.c
                    ├─→ kernel/proc/control.c
                    │       ├─→ kernel/proc/scheduler.c
                    │       └─→ kernel/proc/switch_context.S
                    └─→ include/kernel/proc/globals.h
```

### Tableau récapitulatif de tous les fichiers

| Fichier | Bloc | Lignes | Description |
|---------|------|--------|-------------|
| `kernel/entry.S` | 1 | ~50 | Démarrage assembleur, pile, saut C |
| `kernel/kernel.ld` | 1 | ~25 | Linker script, base `0x80000000` |
| `kernel/io/uart.c` | 1 | ~35 | Pilote UART 16550 MMIO |
| `kernel/io/console.c` | 1 | ~140 | `printf()` minimal %s/%d/%x/%p |
| `kernel/main.c` | 1→2 | ~40 | Point d'entrée C, init séquentielle |
| `include/kernel/types.h` | 2 | ~15 | Types `uint8`…`int64` |
| `include/kernel/mem/layout.h` | 2 | ~25 | `PAGE_SIZE`, `NUM_PAGES`, macros phys↔virt |
| `include/kernel/mem/virtual_memory.h` | 2 | ~35 | Bits PTE, VPN2/1/0, MAKE_PTE |
| `kernel/mem/physmem.c` | 2 | ~105 | Allocateur bitmap |
| `kernel/mem/paging.c` | 2 | ~145 | Tables SV39, walk, map, switch |
| `include/kernel/proc/process.h` | 3 | ~110 | struct proc/context/trapframe, enum |
| `include/kernel/proc/globals.h` | 3 | ~20 | proc_table, current_process, NPROC |
| `kernel/proc/process.c` | 3 | ~140 | alloc/free/init proc |
| `kernel/proc/control.c` | 3 | ~130 | fork/exit/wait/sleep/wakeup |
| `kernel/proc/scheduler.c` | 3 | ~62 | Round-robin scheduler, yield |
| `kernel/proc/switch_context.S` | 3 | ~52 | Context switch RISC-V assembleur |

---

## 6. Flux d'initialisation complet du kernel

```
QEMU démarre
    │
    ↓
_start (entry.S)
    │  csrw mstatus, zero   ← interruptions désactivées
    │  la sp, stack_top     ← pile 16 Ko initialisée
    │  call kernel_main
    │
    ↓
kernel_main() (main.c)
    │
    ├─ uart_init()          ← UART 16550 configuré (Bloc 1)
    │
    ├─ printf(...)          ← "Lithium Kernel starting..."
    │
    ├─ physmem_init()       ← Bitmap 4 Ko initialisée (Bloc 2)
    │       │  Toutes les pages libres (0xFF)
    │       │  Pages kernel marquées allouées
    │       └─ printf("physmem_init: 32768 pages, N libres")
    │
    ├─ paging_init()        ← Tables SV39 créées (Bloc 2)
    │       │  create_page_table() → page root allouée
    │       │  for(0→128Mo) map_page(i, i, R|W|X) → identity map
    │       │  switch_page_table() → csrw satp + sfence.vma
    │       └─ printf("paging_init: activated")
    │
    └─ [Bloc 3 : proc_init() + scheduler() seront ici]
            │  proc_table tous UNUSED
            │  alloc_proc() → init PID=1
            │  physmem_alloc_page() → kstack + trapframe
            │  create_page_table() → pagetable init
            │  init→state = PROC_RUNNABLE
            └─ scheduler() → boucle round-robin infinie
```

---

## 7. Chaîne de compilation et outils

### Makefile – Structure en blocs

```makefile
CROSS_COMPILE = riscv64-unknown-elf-
CC    = $(CROSS_COMPILE)gcc
LD    = $(CROSS_COMPILE)ld
QEMU  = qemu-system-riscv64

# Flags de compilation pour un kernel freestanding
CFLAGS  = -Wall -Werror -O2 -fno-omit-frame-pointer -ggdb -gdwarf-2
CFLAGS += -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib
CFLAGS += -mno-relax -I include

LDFLAGS = -T kernel/kernel.ld -z max-page-size=4096
```

**Explication des flags clés** :

| Flag | Signification |
|------|--------------|
| `-ffreestanding` | Pas de bibliothèque standard (pas de `main`, pas de `libc`) |
| `-nostdlib` | Ne pas linker avec `-lc`, `-lgcc`, etc. |
| `-mcmodel=medany` | Modèle de code pour adresses arbitraires (pas `medlow`) |
| `-mno-relax` | Désactiver les relaxations de l'assembleur (évite des relocations surprises) |
| `-fno-common` | Les variables globales non initialisées vont en `.bss` unique (pas en `COMMON`) |
| `-fno-omit-frame-pointer` | Garder `s0/fp` pour que GDB puisse faire des backtraces |
| `-MD` | Générer des fichiers `.d` de dépendances Makefile automatiquement |

### Commandes disponibles

```bash
# Compiler le kernel (produit kernel/kernel.elf)
make

# Nettoyer tous les fichiers générés
make clean

# Lancer sur QEMU (affichage série dans le terminal)
make qemu

# Lancer en mode debug (QEMU attend GDB sur le port 1234)
make debug
```

### Vérifier l'ELF produit

```bash
# Afficher les sections du binaire
riscv64-unknown-elf-readelf -S kernel/kernel.elf

# Désassembler entry.S pour vérifier le point d'entrée
riscv64-unknown-elf-objdump -d kernel/kernel.elf | head -40

# Vérifier la taille des sections
riscv64-unknown-elf-size kernel/kernel.elf
```

---

## 8. Débogage avec GDB + QEMU

### Lancer une session de débogage

```bash
# Terminal 1 : lancer QEMU en attente de GDB
make debug
# QEMU affiche : Waiting for GDB connection on port 1234...

# Terminal 2 : connecter GDB
gdb-multiarch kernel/kernel.elf
(gdb) target remote :1234
(gdb) break kernel_main
(gdb) continue
```

### Points d'arrêt utiles par bloc

```bash
# Bloc 1 : UART
(gdb) break uart_putchar     # Vérifier que l'UART envoie bien les octets

# Bloc 2 : Mémoire
(gdb) break physmem_init     # Voir l'initialisation de la bitmap
(gdb) break physmem_alloc_page
(gdb) break paging_init      # Vérifier la création de la table root
(gdb) break switch_page_table  # Voir la valeur de satp

# Bloc 3 : Processus
(gdb) break alloc_proc       # Vérifier l'allocation de proc_table[i]
(gdb) break fork             # Observer la création d'un enfant
(gdb) break context_switch   # Inspecter les registres avant/après
(gdb) break scheduler        # Voir l'ordonnanceur choisir un processus
```

### Commandes GDB clés pour le kernel

```bash
# Inspecter les registres RISC-V
(gdb) info registers
(gdb) print $ra              # Return address
(gdb) print $satp            # Registre de pagination

# Inspecter la mémoire
(gdb) x/20gx 0x80000000      # Lire 20 mots 64-bit à l'adresse 0x80000000
(gdb) x/512gx kernel_pagetable  # Afficher la table de pages root

# Inspecter proc_table
(gdb) print proc_table[0]    # Premier processus
(gdb) print *current_process  # Processus courant
(gdb) print current_process->state  # État (0=UNUSED, 3=RUNNABLE...)

# Backtrace
(gdb) bt                     # Pile d'appels
(gdb) frame 2                # Aller au frame 2
(gdb) info locals            # Variables locales du frame courant

# Exécution
(gdb) stepi                  # Exécuter une instruction assembleur
(gdb) nexti                  # Pareil sans entrer dans les fonctions
(gdb) continue               # Continuer jusqu'au prochain breakpoint
(gdb) finish                 # Terminer la fonction courante
```

### Paniques kernel courantes et diagnostics

| Symptôme | Cause probable | Diagnostic |
|----------|----------------|-----------|
| `scause = 0x2` (Illegal Instruction) | Instruction invalide ou mauvais mode | `(gdb) print $pc` puis `disas` |
| `scause = 0xc` (Page Fault Load) | Lecture d'une page non mappée | `(gdb) print $stval` → adresse fautive |
| `scause = 0xd` (Page Fault Store) | Écriture sur page read-only | Vérifier les flags PTE |
| QEMU sans sortie | UART non initialisé ou mauvaise adresse | Vérifier `UART_BASE = 0x10000000` |
| Reboot infini | `sp` mal aligné ou débordement de pile | Vérifier `.align 16` dans `entry.S` |
| `paging_init: failed` | `physmem_alloc_page()` retourne 0 | `print free_pages_count` après `physmem_init` |
| `alloc_proc: no more processes` | `NPROC = 64` dépassé | Augmenter `NPROC` ou libérer des zombies |

---

*Rapport généré sur la branche `gabrielle` – Lithium Kernel – Mai 2026*
