# Rapport de TP – Lithium Kernel
## Comprendre ce qu'on a construit et pourquoi

**Étudiante** : Gabrielle Nana  
**Rôle dans le projet** : Développeuse mémoire – Bitmap, Paging, Tables de pages  
**Projet** : Lithium Kernel – noyau minimal RISC-V  
**Équipe** : Bala Andegue, Gabrielle Nana, Israel Teme, Tamwo Steve  
**Superviseur** : Professeur Alain Tchana  
**Date** : Mai 2026  

---

## Table des matières

1. [C'est quoi ce projet en résumé ?](#1-cest-quoi-ce-projet-en-résumé-)
2. [Les outils qu'on utilise](#2-les-outils-quon-utilise)
3. [Bloc 1 – Faire démarrer le kernel](#3-bloc-1--faire-démarrer-le-kernel)
4. [Bloc 2 – Gérer la mémoire](#4-bloc-2--gérer-la-mémoire)
5. [Bloc 3 – Gérer les processus](#5-bloc-3--gérer-les-processus)
6. [Comment tout s'assemble](#6-comment-tout-sassemble)
7. [Ce que j'ai personnellement appris](#7-ce-que-jai-personnellement-appris)

---

## 1. C'est quoi ce projet en résumé ?

### L'idée de base

Quand tu utilises un ordinateur, tu lances des applications : un navigateur, un éditeur de texte, un jeu. Ces applications **ne parlent pas directement au matériel** (le processeur, la mémoire, le disque). Il y a un intermédiaire entre elles et le matériel : c'est le **noyau** (ou *kernel* en anglais).

Le noyau est le programme le plus fondamental d'un ordinateur. Il démarre en premier, il s'occupe de gérer la mémoire, de lancer les programmes, de leur donner du temps processeur, etc.

Dans ce TP, on a **construit notre propre noyau de zéro**, qu'on a appelé **Lithium Kernel**. Ce n'est pas un vrai noyau comme Linux – c'est une version simplifiée, pédagogique, pour comprendre comment ça marche de l'intérieur.

### Le matériel qu'on cible

Notre kernel tourne sur une architecture **RISC-V 64 bits**. RISC-V est un processeur « open source » (dont les plans sont publics), très utilisé dans les cours d'OS car sa conception est simple et bien documentée.

On n'a pas de vrai processeur RISC-V sous la main, donc on utilise **QEMU** : c'est un émulateur, un programme qui simule un ordinateur RISC-V sur notre PC habituel. C'est comme jouer à un jeu vidéo d'ordinateur à l'intérieur d'un ordinateur.

```
Notre PC (x86/AMD64)
  └── QEMU (émulateur)
         └── Machine virtuelle RISC-V 64 bits
                └── Notre kernel Lithium
```

### Ce qu'on a construit en 3 blocs

```
┌─────────────────────────────────────────────────┐
│  BLOC 3 – Processus                             │
│  Gérer plusieurs programmes en même temps       │
├─────────────────────────────────────────────────┤
│  BLOC 2 – Mémoire (mon rôle principal)          │
│  Distribuer la RAM, créer l'espace virtuel      │
├─────────────────────────────────────────────────┤
│  BLOC 1 – Démarrage                             │
│  Allumer le kernel, afficher du texte           │
└─────────────────────────────────────────────────┘
         ↕ tout tourne sur ↕
┌─────────────────────────────────────────────────┐
│  MATÉRIEL RISC-V (émulé par QEMU)               │
│  CPU, RAM 128 Mo, port série UART               │
└─────────────────────────────────────────────────┘
```

---

## 2. Les outils qu'on utilise

### Le langage C

On écrit le kernel en **langage C**. C est un langage de bas niveau : on gère soi-même la mémoire, on peut accéder directement aux adresses mémoire, on peut écrire dans des registres hardware. C'est pour ça que tous les noyaux (Linux, Windows, macOS) sont écrits en C.

### L'assembleur RISC-V

Pour quelques parties très critiques (comme le démarrage et le changement de processus), on écrit en **assembleur** : c'est le langage le plus proche du matériel, où chaque ligne correspond à une seule instruction du processeur.

### Le cross-compilateur

On compile notre code C sur un PC Linux classique, mais le code doit tourner sur un processeur RISC-V. On utilise donc un **cross-compilateur** : `riscv64-unknown-elf-gcc`. Il traduit notre C en instructions RISC-V.

```bash
# Ce qu'on tape pour compiler :
make

# Ce que make fait en coulisse :
riscv64-unknown-elf-gcc -ffreestanding -nostdlib ... kernel/main.c -o kernel/main.o
riscv64-unknown-elf-ld -T kernel/kernel.ld -o kernel/kernel.elf ...
```

### xv6 comme inspiration

On s'est inspiré du noyau éducatif **xv6** du MIT (Massachusetts Institute of Technology). C'est un noyau minimaliste créé pour enseigner les systèmes d'exploitation, qu'on a réécrit et adapté à notre façon.

---

## 3. Bloc 1 – Faire démarrer le kernel

### Pourquoi c'est compliqué de démarrer ?

Quand on allume un ordinateur, le processeur démarre dans un état très basique : pas de pile, pas de mémoire configurée, pas d'affichage. Il exécute la première instruction à une adresse fixée à l'avance (`0x80000000` sur notre QEMU).

La première chose à faire c'est de **préparer l'environnement minimum** pour que du code C puisse s'exécuter.

### Fichier `kernel/entry.S` – Le point de départ absolu

C'est le fichier assembleur qui s'exécute en **tout premier**, avant même le code C.

```asm
_start:
    csrw mstatus, zero   # Désactiver toutes les interruptions
    la   sp, stack_top   # Dire au CPU où est la pile
    call kernel_main     # Sauter vers le code C
```

**Ligne par ligne :**

- `csrw mstatus, zero` : On écrit 0 dans le registre `mstatus` (Machine Status). Cela désactive les interruptions. Pourquoi ? Parce qu'au démarrage le kernel n'est pas encore prêt à gérer des interruptions – si une interruption arrive trop tôt, ça plante tout.

- `la sp, stack_top` : `sp` est le registre "Stack Pointer" (pointeur de pile). La **pile** est une zone mémoire où les fonctions C stockent leurs variables locales. Sans pile, pas de fonctions C possibles. On dit au CPU où elle se trouve.

- `call kernel_main` : On saute vers la première fonction C, `kernel_main()`.

**Qu'est-ce qu'une pile ?**  
Imagine une pile d'assiettes. Quand une fonction démarre, elle "pose une assiette" (réserve de la place pour ses variables). Quand elle se termine, elle "enlève son assiette". La pile grandit vers le bas en mémoire sur RISC-V.

```
Adresse haute ──→ stack_top  ← sp démarre ici
                  [variables de kernel_main]
                  [variables de physmem_init]
                  [variables de mark_page_allocated]
Adresse basse ──→ (fond de pile – 16 Ko réservés)
```

### Fichier `kernel/kernel.ld` – Le script de l'éditeur de liens

Quand on compile plusieurs fichiers `.c`, le **linker** (éditeur de liens) les assemble en un seul exécutable. Ce fichier `.ld` lui dit comment :

```
ENTRY(_start)           ← commencer à _start
BASE_ADDRESS = 0x80000000  ← charger le kernel à cette adresse

SECTIONS {
    .text   → le code du kernel
    .data   → les variables globales initialisées
    .bss    → les variables globales = 0
    _end = . ← marquer la fin du kernel (utilisé en Bloc 2)
}
```

**Pourquoi `0x80000000` ?** C'est l'adresse que QEMU utilise pour charger un kernel. C'est une convention fixée par les concepteurs de QEMU pour la machine virtuelle `virt`.

### Fichier `kernel/io/uart.c` – Afficher du texte

Dans un kernel, il n'y a pas de `printf()` de la librairie standard. On n'a que le matériel brut. Pour afficher du texte, on utilise l'**UART** (Universal Asynchronous Receiver/Transmitter) : c'est le composant qui gère la communication série (le port série).

Sur QEMU, l'UART est accessible à l'adresse mémoire `0x10000000`. Pour lui envoyer un caractère, on **écrit directement à cette adresse mémoire** :

```c
void uart_putchar(char c)
{
    // Attendre que l'UART soit prêt (bit 5 du registre LSR = 1)
    while (!(*(volatile uint8*)(0x10000000 + 5) & (1 << 5)))
        ;   // On tourne en boucle jusqu'à ce qu'il soit libre

    // Envoyer le caractère
    *(volatile uint8*)(0x10000000) = c;
}
```

**C'est quoi `volatile` ?** Sans ce mot, le compilateur pourrait "optimiser" et ne pas vraiment écrire en mémoire (il penserait que c'est inutile). `volatile` lui dit : "cette adresse mémoire contrôle du matériel réel, exécute vraiment l'instruction".

**C'est quoi MMIO ?** Memory-Mapped I/O : les périphériques (UART, timer, etc.) sont contrôlés en lisant et écrivant à des adresses mémoire spéciales. C'est la technique universelle dans les systèmes embarqués.

### Fichier `kernel/io/console.c` – Notre propre `printf()`

Comme on n'a pas la librairie C standard, on a réécrit `printf()` de zéro. Il supporte uniquement les formats dont on a besoin :

| Format | Signification | Exemple |
|--------|---------------|---------|
| `%s` | Afficher une chaîne | `"hello"` |
| `%d` | Afficher un entier décimal | `42` |
| `%x` | Afficher en hexadécimal | `0x80001000` |
| `%p` | Afficher un pointeur | `0x80054000` |

**Comment ça marche sans `<stdio.h>` ?**  
On accède aux arguments variables avec un pointeur arithmétique :
```c
char **arg = (char**)(&fmt + 1);  // pointe juste après fmt sur la pile
```
Puis pour chaque `%d`, `%s`, etc., on lit la valeur et on avance le pointeur.

### Résultat du Bloc 1

Quand on lance `make qemu`, on voit apparaître dans le terminal :
```
Lithium Kernel starting...
========================================
```
Le kernel a démarré, et il peut communiquer avec nous. C'est une grande victoire !

---

## 4. Bloc 2 – Gérer la mémoire

*C'est mon bloc principal dans le projet.*

### Pourquoi gérer la mémoire ?

Le kernel a besoin d'allouer de la mémoire constamment : pour créer des processus, pour les tables de pages, pour les piles. Il faut un système qui sache **quelles zones de RAM sont libres** et qui puisse en donner à la demande.

De plus, on a besoin d'un concept de **mémoire virtuelle** : chaque processus doit avoir l'illusion d'avoir tout l'espace mémoire pour lui seul, alors qu'en réalité ils se partagent la même RAM physique.

### La RAM disponible

Sur notre QEMU, on a **128 Mo de RAM** à partir de l'adresse `0x80000000`. C'est là que le kernel lui-même est chargé.

```
0x80000000  ┌──────────────────────┐
            │   Code du kernel     │  ← .text, .data, .bss
            │   (_start à _end)    │
            ├──────────────────────┤
            │   Pages libres       │  ← physmem_alloc_page() distribue d'ici
            │   (gérées par        │
            │    notre allocateur) │
0x88000000  └──────────────────────┘  (128 Mo plus tard)
```

### L'unité de base : la page mémoire

On ne gère pas la mémoire octet par octet – ce serait beaucoup trop lent. On la gère par **pages** de **4096 octets (4 Ko)** chacune.

```
128 Mo ÷ 4 Ko = 32 768 pages au total
```

Chaque page a un numéro (son index) et une adresse physique : la page numéro 5 est à l'adresse `5 × 4096 = 20480 = 0x5000`.

### Fichier `kernel/mem/physmem.c` – L'allocateur de pages physiques

#### L'idée de la bitmap

Pour savoir quelles pages sont libres, on utilise une **bitmap** : un tableau de bits où **chaque bit représente une page**.

- Bit = **1** → page **libre**
- Bit = **0** → page **allouée**

32 768 pages → 32 768 bits → **4 096 octets (4 Ko)** pour toute la bitmap. C'est très compact !

```
bitmap[0] = 11111111  → les pages 0 à 7 sont libres
bitmap[1] = 00001111  → pages 8-11 allouées, pages 12-15 libres
bitmap[2] = 11111111  → pages 16 à 23 libres
...
```

#### Comment on accède à un bit précis ?

Pour la page numéro `i` :
- Elle est dans l'octet numéro `i / 8` de la bitmap
- C'est le bit numéro `i % 8` dans cet octet

Exemple pour la page 13 :
```
byte_index = 13 / 8 = 1      → octet bitmap[1]
bit_index  = 13 % 8 = 5      → bit numéro 5
```

Pour marquer la page 13 comme allouée (mettre son bit à 0) :
```c
bitmap[1] &= ~(1 << 5);
// bitmap[1] avant : 00001111
// (1 << 5)        : 00100000
// ~(1 << 5)       : 11011111
// bitmap[1] après : 00001111 & 11011111 = 00001111  (bit 5 était déjà 0)
```

#### La fonction `physmem_init()`

Au démarrage, on initialise la bitmap :

```c
void physmem_init(void)
{
    // 1. Marquer TOUTES les pages comme libres
    for (i = 0; i < NUM_PAGES / 8; i++)
        bitmap[i] = 0xFF;   // 0xFF = 11111111 = toutes libres

    // 2. Trouver où se termine le kernel (symbole _end du linker)
    uint64 kernel_end = (uint64)&_end;
    uint64 premiere_page_libre = (kernel_end + 4096 - 1) / 4096;

    // 3. Marquer les pages du kernel comme allouées
    for (i = 0; i < premiere_page_libre; i++)
        mark_page_allocated(i);
}
```

**Pourquoi protéger les pages du kernel ?** Si on allouait une page déjà utilisée par le code du kernel et qu'on y écrivait des données, on écraserait des instructions, ce qui planterait tout. Il faut donc d'abord "réserver" les pages occupées par le kernel.

#### Allouer et libérer une page

```c
// Allouer : chercher le premier bit = 1 dans la bitmap
uint64 physmem_alloc_page(void)
{
    for (i = 0; i < NUM_PAGES; i++) {
        if (is_page_free(i)) {
            mark_page_allocated(i);
            return i * PAGE_SIZE;  // retourner l'adresse physique
        }
    }
    return 0;  // plus de mémoire !
}

// Libérer : remettre le bit à 1
void free_physical_page(uint64 adresse_physique)
{
    uint64 i = adresse_physique / PAGE_SIZE;
    mark_page_free(i);
}
```

### C'est quoi la mémoire virtuelle ?

Voici un problème concret : si on a 2 programmes, le programme A et le programme B, et que les deux veulent utiliser l'adresse mémoire `0x1000` pour stocker leur variable `x`, ils vont se marcher dessus !

La solution : la **mémoire virtuelle**. Chaque programme voit son propre espace d'adressage fictif (`0x0000` à `0xFFFFFFFF`), et le kernel traduit ces adresses fictives en vraies adresses physiques différentes.

```
Programme A croit écrire à 0x1000  ──→  en réalité → physique 0x80060000
Programme B croit écrire à 0x1000  ──→  en réalité → physique 0x80070000
```

Cette traduction est faite par le **MMU** (Memory Management Unit), un circuit dans le processeur, en utilisant des **tables de pages**.

### L'architecture SV39 (la pagination RISC-V)

RISC-V 64 bits utilise un mode de pagination appelé **SV39** :
- **S**upervisor
- **V**irtual address of
- **39** bits

Cela signifie que les adresses virtuelles font 39 bits, soit un espace adressable de 512 Go.

#### Comment une adresse virtuelle est découpée

```
Adresse virtuelle de 39 bits :
┌──────────┬──────────┬──────────┬────────────┐
│  VPN[2]  │  VPN[1]  │  VPN[0]  │   Offset   │
│  9 bits  │  9 bits  │  9 bits  │  12 bits   │
│ bits 38-30│bits 29-21│bits 20-12│ bits 11-0  │
└──────────┴──────────┴──────────┴────────────┘
     ↓            ↓           ↓
 Index dans   Index dans  Index dans
  Table L2     Table L1    Table L0
```

- **VPN** = Virtual Page Number (numéro de page virtuelle)
- **Offset** = position dans la page (0 à 4095)

Chaque niveau de table contient 512 entrées (9 bits → 2⁹ = 512). Il y a **3 niveaux** de tables, d'où le nom "3-level page tables".

#### Comment fonctionne la traduction en 3 étapes

```
Adresse virtuelle : 0x80002000

Étape 1 : VPN[2] = (0x80002000 >> 30) & 0x1FF = 0
          → aller chercher table_L2[0]
          → cette entrée pointe vers Table_L1 en mémoire physique

Étape 2 : VPN[1] = (0x80002000 >> 21) & 0x1FF = 0
          → aller chercher Table_L1[0]
          → cette entrée pointe vers Table_L0 en mémoire physique

Étape 3 : VPN[0] = (0x80002000 >> 12) & 0x1FF = 2
          → aller chercher Table_L0[2]
          → cette entrée donne l'adresse physique de la page

Résultat : adresse physique = adresse_dans_L0 + offset (0x000)
```

Chaque "table" est simplement une page mémoire (4 Ko) contenant 512 entrées de 8 octets chacune.

#### Les bits de permission dans une entrée (PTE)

Chaque entrée de table de pages (*PTE = Page Table Entry*) contient non seulement l'adresse de la page physique, mais aussi des **bits de permission** :

```
Bit 0 (V = Valid)   : l'entrée est-elle valide ?
Bit 1 (R = Read)    : peut-on lire cette page ?
Bit 2 (W = Write)   : peut-on écrire dans cette page ?
Bit 3 (X = Execute) : peut-on exécuter le code de cette page ?
Bit 4 (U = User)    : les programmes utilisateur peuvent-ils y accéder ?
```

Exemple : une page de code kernel → `R=1, W=1, X=1, U=0` (le code peut être lu, écrit, exécuté, mais pas par les programmes utilisateur).

### Fichier `kernel/mem/paging.c` – Les tables de pages en pratique

#### Créer une table de pages

```c
void* create_page_table(void)
{
    uint64 phys = physmem_alloc_page();  // allouer 1 page physique
    uint64 *table = (uint64*)phys;
    for (int i = 0; i < 512; i++)
        table[i] = 0;                   // toutes les entrées invalides au début
    return table;
}
```

#### Mapper une page virtuelle sur une page physique

```c
int map_page(void *table, uint64 adresse_virtuelle,
             uint64 adresse_physique, uint64 flags)
{
    // walk() descend les 3 niveaux et retourne un pointeur vers la PTE finale
    uint64 *pte = walk(table, adresse_virtuelle, 1);  // alloc=1 : créer les niveaux

    // Écrire l'adresse physique + les permissions dans la PTE
    *pte = adresse_physique | flags | PTE_V;
}
```

#### L'identity mapping au démarrage

Au démarrage, on fait un **identity mapping** : on mappe chaque adresse virtuelle sur la même adresse physique.

```c
void paging_init(void)
{
    kernel_pagetable = create_page_table();

    // Pour chaque page de 0 à 128 Mo :
    for (i = 0; i < 128_Mo; i += 4096) {
        map_page(kernel_pagetable, i, i, R | W | X);
        //                         ↑  ↑
        //             même virt = phys (identity)
    }

    // Activer la pagination en écrivant dans le registre SATP
    switch_page_table(kernel_pagetable);
}
```

**Pourquoi identity mapping ?** Parce qu'au moment où on active la pagination, le CPU est déjà en train d'exécuter du code à l'adresse `0x80001234` (par exemple). Si on activait une pagination qui dit "l'adresse virtuelle `0x80001234` correspond à physique `0xDEADBEEF`", la prochaine instruction serait lue depuis `0xDEADBEEF` et tout planterait. Avec l'identity mapping, les adresses virtuelles = adresses physiques, donc rien ne change pour le code déjà en cours d'exécution.

#### Activer la pagination : le registre SATP

Pour activer la pagination sur RISC-V, on écrit dans le registre spécial **SATP** (Supervisor Address Translation and Protection) :

```c
void switch_page_table(void *table_root)
{
    uint64 satp = (8UL << 60)           // Mode = 8 → SV39
                | (adresse_physique >> 12);  // Numéro de la page root

    asm volatile("csrw satp, %0" : : "r"(satp));  // Écrire dans SATP
    asm volatile("sfence.vma zero, zero");          // Vider le cache TLB
}
```

**C'est quoi le TLB ?** Le TLB (Translation Lookaside Buffer) est un cache dans le CPU qui mémorise les traductions récentes pour aller plus vite. Quand on change de table de pages, il faut le vider (`sfence.vma`) sinon le CPU continuerait à utiliser d'anciennes traductions incorrectes.

### Résultat du Bloc 2

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

Le kernel peut maintenant :
- Allouer et libérer des pages mémoire à la demande
- Gérer la traduction d'adresses virtuelles → physiques
- Isoler les espaces mémoire des différents programmes (futur)

---

## 5. Bloc 3 – Gérer les processus

### C'est quoi un processus ?

Un **processus** c'est un programme en cours d'exécution. Quand tu lances un programme, le système crée un processus pour lui : il lui alloue de la mémoire, lui donne un numéro d'identification (PID), et le met en file d'attente pour avoir du temps processeur.

Dans notre kernel, un processus est représenté par une structure C qui contient toutes les informations dont le kernel a besoin pour le gérer.

### Fichier `include/kernel/proc/process.h` – La structure d'un processus

#### Les états d'un processus

Un processus passe par différents états au cours de sa vie :

```
                    alloc_proc()
                        │
                        ↓
   ┌─────────────────►UNUSED◄──────────────────────┐
   │                                free_proc()     │
   │                 PROC_USED                      │
   │                    │ (proc_init ou fork)        │
   │                    ↓                           │
   │            ┌──►RUNNABLE◄──┐                    │
   │            │    │          │ wakeup()           │
   │            │    │ scheduler│                    │
   │            │    ↓          │                    │
   │    yield() │  RUNNING      │                    │
   │            │    │          │                    │
   │            │    │sleep()   │                    │
   │            │    ↓          │                    │
   │            │  SLEEPING─────┘                    │
   │            │    │                               │
   │            │    │ exit()                        │
   │            │    ↓                               │
   └────────────┴─ ZOMBIE ──────────── wait() ───────┘
```

| État | Signification |
|------|---------------|
| `PROC_UNUSED` | Case libre dans la table – pas de processus ici |
| `PROC_USED` | Structure réservée, mais pas encore prêt |
| `PROC_RUNNABLE` | Prêt à s'exécuter, attend son tour |
| `PROC_RUNNING` | En cours d'exécution sur le CPU |
| `PROC_SLEEPING` | Endormi, attend un événement |
| `PROC_ZOMBIE` | Terminé mais parent pas encore récupéré le code de sortie |

#### La structure `proc`

```c
struct proc {
    enum proc_state state;      // État actuel (RUNNABLE, RUNNING, etc.)
    int pid;                    // Numéro unique du processus (1, 2, 3...)
    struct proc *parent;        // Qui a créé ce processus ?
    void *pagetable;            // Sa table de pages (son espace virtuel)
    uint64 sz;                  // Combien de mémoire il utilise
    uint64 kstack;              // Sa pile noyau (pour les appels système)
    struct trapframe *trapframe; // Ses registres sauvegardés (pour reprendre)
    struct context context;     // Ses registres CPU pour le context switch
    int exit_code;              // Valeur retournée à exit()
    char name[32];              // Son nom (pour le débogage)
};
```

#### La structure `context` – ce qu'on sauvegarde lors d'un changement de processus

```c
struct context {
    uint64 ra;   // Adresse de retour (où reprendre l'exécution)
    uint64 sp;   // Pointeur de pile (quelle pile utiliser)
    uint64 s0;   // \
    uint64 s1;   //  \
    uint64 s2;   //   \  Registres "sauvegardés" selon la
    uint64 s3;   //   /  convention d'appel RISC-V
    uint64 s4;   //  /   (le callee doit les préserver)
    uint64 s5;   // /
    uint64 s6;
    uint64 s7;
    uint64 s8;
    uint64 s9;
    uint64 s10;
    uint64 s11;
};
```

### Fichier `kernel/proc/process.c` – Créer et détruire des processus

#### La table de processus

Tous les processus sont stockés dans un tableau global de 64 entrées :

```c
struct proc proc_table[64];   // Maximum 64 processus simultanés
```

C'est simple et efficace : pas besoin d'allocation dynamique complexe.

#### `alloc_proc()` – Réserver une place dans la table

```c
struct proc* alloc_proc(void)
{
    for (int i = 0; i < 64; i++) {
        if (proc_table[i].state == PROC_UNUSED) {
            // Trouver une case libre et l'initialiser
            proc_table[i].state = PROC_USED;
            proc_table[i].pid   = next_pid++;  // PID unique
            return &proc_table[i];
        }
    }
    return NULL;  // Plus de place !
}
```

#### `proc_init()` – Créer le premier processus (init, PID=1)

C'est le processus ancêtre de tous les autres. Sur un vrai Linux, c'est `/sbin/init` (ou `systemd`). Dans notre kernel, c'est un processus factice créé directement :

```c
void proc_init(void)
{
    struct proc *init = alloc_proc();   // PID = 1

    // Lui allouer une pile noyau (1 page de 4 Ko)
    uint64 kstack = physmem_alloc_page();
    init->kstack = kstack + PAGE_SIZE;  // sp pointe vers le HAUT

    // Lui allouer un trapframe (pour sauvegarder ses registres)
    init->trapframe = (struct trapframe*)physmem_alloc_page();

    // Lui créer sa table de pages
    init->pagetable = create_page_table();

    // Il est prêt à tourner !
    init->state = PROC_RUNNABLE;
}
```

### Fichier `kernel/proc/control.c` – Les opérations classiques UNIX

#### `fork()` – Dupliquer un processus

`fork()` est une des fonctions les plus importantes des systèmes UNIX. Elle crée un **processus enfant** qui est une copie du processus parent.

```c
int fork(void)
{
    struct proc *parent = current_process;
    struct proc *child  = alloc_proc();     // Nouvelle case dans la table

    child->parent = parent;               // Lien parent-enfant
    child->sz     = parent->sz;           // Même taille mémoire

    // Copier les registres du parent dans l'enfant
    *child->trapframe = *parent->trapframe;
    child->trapframe->a0 = 0;             // fork() retourne 0 dans l'enfant
                                          // et le PID de l'enfant dans le parent

    child->pagetable = create_page_table();
    child->state     = PROC_RUNNABLE;

    return child->pid;    // Le parent reçoit le PID de l'enfant
}
```

**Pourquoi `fork()` retourne deux valeurs différentes ?**
```
Parent appelle fork() :
  - Dans le parent : fork() retourne 5 (PID de l'enfant)
  - Dans l'enfant  : fork() retourne 0

Comment ? On écrit a0 = 0 dans le trapframe de l'enfant.
a0 est le registre qui contient la valeur de retour d'une fonction en RISC-V.
Quand l'enfant reprend son exécution depuis le trapframe, il "croit" que fork() a retourné 0.
```

#### `exit()` – Terminer un processus

```c
void exit(int status)
{
    struct proc *p = current_process;

    p->exit_code = status;       // Sauvegarder le code de sortie
    p->state = PROC_ZOMBIE;      // Devenir zombie (plus d'exécution mais pas libéré)

    wakeup(p->parent);           // Réveiller le parent qui attendait peut-être dans wait()

    while (1) asm("wfi");        // Attendre sans consommer le CPU
}
```

**Pourquoi "zombie" ?** On ne peut pas libérer immédiatement les ressources car le parent doit encore pouvoir lire le code de sortie. L'état ZOMBIE signifie "terminé mais pas encore ramassé par le parent". C'est le même concept que dans les vrais systèmes UNIX.

#### `wait()` – Le parent attend la fin d'un enfant

```c
int wait(uint64 adresse_du_statut)
{
    for (int i = 0; i < 64; i++) {
        if (proc_table[i].parent == current_process
            && proc_table[i].state == PROC_ZOMBIE) {
            // Trouver un enfant zombie
            int pid = proc_table[i].pid;
            *(int*)adresse_du_statut = proc_table[i].exit_code;  // copier le statut
            free_proc(&proc_table[i]);  // libérer les ressources
            return pid;
        }
    }
    return -1;  // pas d'enfant zombie
}
```

#### `sleep()` et `wakeup()` – Attendre un événement

```c
void sleep(void *canal)
{
    current_process->state = PROC_SLEEPING;
    scheduler();          // Rendre la main, on ne reprend que quand quelqu'un nous réveille
}

void wakeup(void *canal)
{
    for (int i = 0; i < 64; i++) {
        if (proc_table[i].state == PROC_SLEEPING)
            proc_table[i].state = PROC_RUNNABLE;   // Réveiller !
    }
}
```

### Fichier `kernel/proc/scheduler.c` – L'ordonnanceur

L'ordonnanceur (scheduler) décide **quel processus tourne à quel moment**. Notre implémentation utilise l'algorithme le plus simple : le **round-robin**.

**Round-robin** = chacun son tour, en boucle. Comme une ronde d'enfants.

```
Processus : [A][B][C][D]
Tour 1 : A tourne → yield() ou bloqué
Tour 2 : B tourne → yield() ou bloqué
Tour 3 : C tourne → yield() ou bloqué
Tour 4 : D tourne → yield() ou bloqué
Tour 5 : A tourne à nouveau...
```

```c
void scheduler(void)
{
    static int index = 0;   // Se souvenir où on en était

    while (1) {
        for (int i = 0; i < 64; i++) {
            struct proc *p = &proc_table[index];
            index = (index + 1) % 64;      // Passer au suivant (en boucle)

            if (p->state == PROC_RUNNABLE) {
                current_process = p;
                p->state = PROC_RUNNING;
                switch_page_table(p->pagetable);     // Charger sa mémoire virtuelle
                context_switch(&prev->context, &p->context);   // Changer de contexte
            }
        }
    }
}
```

### Fichier `kernel/proc/switch_context.S` – Le cœur du changement de processus

C'est le fichier le plus technique du projet. Il est écrit en **assembleur** parce qu'il doit manipuler directement les registres du processeur.

**Le problème :** quand on passe du processus A au processus B, les registres du CPU (ra, sp, s0-s11) contiennent les données de A. Il faut les sauvegarder pour A, puis charger ceux de B.

```asm
context_switch:
    # a0 = pointeur vers le contexte de l'ANCIEN processus (à sauvegarder)
    # a1 = pointeur vers le contexte du NOUVEAU processus (à restaurer)

    # ── Sauvegarder les registres de l'ancien processus ──
    sd ra,    0(a0)    # "store doubleword" : écrire ra à l'adresse a0+0
    sd sp,    8(a0)    # écrire sp à l'adresse a0+8
    sd s0,   16(a0)    # et ainsi de suite...
    sd s1,   24(a0)
    # ... s2 à s11 ...
    sd s11, 104(a0)

    # ── Charger les registres du nouveau processus ──
    ld ra,    0(a1)    # "load doubleword" : lire depuis a1+0 dans ra
    ld sp,    8(a1)    # maintenant sp pointe vers la pile du nouveau processus !
    ld s0,   16(a1)
    ld s1,   24(a1)
    # ... s2 à s11 ...
    ld s11, 104(a1)

    ret    # sauter à l'adresse contenue dans ra
           # = reprendre le nouveau processus là où il s'était arrêté
```

**Ce qui se passe concrètement :**

```
Avant le context_switch :
  CPU exécute le processus A
  ra = adresse dans le scheduler (pour A)
  sp = pile noyau de A

Pendant context_switch :
  On écrit ra, sp, s0-s11 de A dans A->context
  On charge ra, sp, s0-s11 depuis B->context
  → maintenant sp = pile noyau de B !

Après "ret" :
  Le CPU saute à ra qui est maintenant l'adresse de retour de B
  → le processus B reprend exactement là où il s'était arrêté
```

### Résultat du Bloc 3

```
Lithium Kernel starting...
========================================
physmem_init: 32768 pages total, 32544 pages free
paging_init: kernel page table activated
proc_init: initializing process subsystem
proc_init: init process PID=1
scheduler: entering main loop
```

Le kernel peut maintenant :
- Créer des processus (`fork`)
- Les faire se terminer (`exit`) et récupérer leur statut (`wait`)
- Les endormir (`sleep`) et les réveiller (`wakeup`)
- Les alterner sur le CPU de façon équitable (round-robin)
- Sauvegarder et restaurer leur état CPU (`context_switch`)

---

## 6. Comment tout s'assemble

### La séquence de démarrage complète

```
QEMU démarre
   ↓
1. CPU saute à 0x80000000
   ↓
2. entry.S (_start)
   - Désactiver interruptions (csrw mstatus, zero)
   - Initialiser la pile (la sp, stack_top)
   - Sauter vers C (call kernel_main)
   ↓
3. kernel_main()
   ↓
4. uart_init()
   - Préparer le port série pour afficher du texte
   ↓
5. printf("Lithium Kernel starting...")
   ↓
6. physmem_init()
   - Initialiser la bitmap de 4 Ko
   - Marquer les pages du kernel comme occupées
   - ~32 500 pages libres disponibles
   ↓
7. paging_init()
   - Créer la table de pages du kernel
   - Mapper les 128 Mo en identity mapping
   - Activer la pagination (csrw satp + sfence.vma)
   ↓
8. proc_init()
   - Initialiser la table de 64 processus
   - Créer le processus init (PID=1) avec sa pile, son trapframe, sa pagetable
   ↓
9. scheduler()
   - Boucle infinie : chercher un processus RUNNABLE et lui donner le CPU
   - init tourne, peut fork() des enfants...
```

### Les dépendances entre blocs

```
Bloc 3 (Processus)
  dépend de → Bloc 2 (Mémoire) pour allouer pages (kstack, trapframe, pagetable)
  dépend de → Bloc 2 (Paging) pour switch_page_table() lors du context switch
  dépend de → Bloc 1 (UART/printf) pour afficher les messages de debug

Bloc 2 (Mémoire)
  dépend de → Bloc 1 (UART/printf) pour afficher les messages
  dépend de → kernel.ld pour le symbole _end

Bloc 1 (Boot)
  ne dépend de rien d'autre → c'est la fondation
```

---

## 7. Ce que j'ai personnellement appris

### Les concepts que j'ai compris en faisant ce projet

**1. Un ordinateur démarre dans un état très primitif**  
Avant `kernel_main`, il n'y a ni pile, ni affichage, ni mémoire organisée. Tout ce qu'on croit acquis dans un programme C ordinaire doit être mis en place à la main.

**2. La mémoire est une ressource à gérer explicitement**  
Dans un kernel, il n'y a pas de `malloc()` de la librairie standard. On gère les pages une par une. Si on oublie de libérer, la mémoire est perdue pour toujours.

**3. La mémoire virtuelle c'est une illusion organisée**  
Chaque processus croit avoir toute la mémoire pour lui. En réalité le MMU traduit les adresses en coulisse. C'est transparent mais demande une infrastructure complexe (3 niveaux de tables de pages).

**4. Un processus n'est qu'une structure de données**  
Tout l'état d'un processus – ses registres, sa mémoire, son état d'exécution – tient dans un `struct proc`. Le scheduler n'est qu'une boucle qui choisit quelle structure activer.

**5. L'assembleur est inévitable aux frontières**  
Pour certaines opérations (lire/écrire des registres du CPU comme `satp`, `mstatus`, sauvegarder l'état du CPU lors d'un context switch), le C ne suffit pas. Il faut descendre à l'assembleur.

**6. Le débogage sans printf est un art**  
Dans un kernel, si `printf()` ne fonctionne pas encore (ou si le bug arrive avant son initialisation), on ne peut rien afficher. On utilise GDB + QEMU pour arrêter l'exécution et inspecter les registres instruction par instruction.

### Ce que fait ce kernel par rapport à Linux

| Fonctionnalité | Linux | Lithium Kernel |
|----------------|-------|----------------|
| Démarrage | ✅ Complexe (BIOS, GRUB...) | ✅ Simple (QEMU direct) |
| Mémoire physique | ✅ Buddy + slab | ✅ Bitmap simple |
| Mémoire virtuelle | ✅ SV39/SV48, COW, swap | ✅ SV39, identity map |
| Processus | ✅ Millions possible | ✅ 64 max (statique) |
| Scheduler | ✅ CFS (Completely Fair Scheduler) | ✅ Round-robin simple |
| Fork/Exit/Wait | ✅ Complet | ✅ Basique |
| Appels système | ✅ ~350 syscalls | ❌ (Bloc 4, pas encore fait) |
| Système de fichiers | ✅ ext4, btrfs... | ❌ (Bloc 5, pas encore fait) |
| Espace utilisateur | ✅ Isolé | ❌ (Bloc 6, pas encore fait) |

---

*Rapport rédigé par Gabrielle Nana – Branche `gabrielle` – Lithium Kernel – Mai 2026*
