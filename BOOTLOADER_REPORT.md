# Rapport : Implémentation du Bootloader RISC-V pour Lithium Kernel

**Date** : 25 juillet 2026  
**Projet** : Lithium Kernel - Noyau minimal UNIX-like pour RISC-V  
**Objectif** : Implémenter un bootloader minimal pour démarrer le noyau sur QEMU virt

---

## 1. Contexte initial

### État avant l'implémentation
- Le projet Lithium Kernel démarrait directement sans bootloader séparé
- Le noyau était lancé directement par QEMU via l'option `-kernel`
- Pas de distinction entre firmware et code noyau
- Documentation README confirmait : "Bootloader: None (direct kernel boot via `-kernel`)"

### Objectif
- Ajouter un bootloader minimal (stage 1)
- Créer une séquence de démarrage à deux étapes :
  1. Bootloader initialise le CPU et affiche un diagnostic
  2. Bootloader saute vers le noyau chargé à `0x80000000`
- Tester le tout avec QEMU

---

## 2. Implémentation du bootloader

### 2.1 Fichier : boot/boot.S

**Description** : Point d'entrée du bootloader en assembleur RISC-V

**Fonctionnalités principales** :
- Désactive les interruptions machine (`csrw mstatus, zero`)
- Initialise la pile RISC-V à partir de `stack_top`
- Affiche le message "BOOTLOADER OK" via l'UART
- Saute à `0x80000000` pour exécuter le noyau
- Boucle infinie de secours (`hang`) en cas de retour anormal

**Points clés du code** :

```assembly
# Désactiver les interruptions
csrw mstatus, zero

# Initialiser la pile
la sp, stack_top

# Afficher le message
la a0, boot_msg
call uart_print

# Sauter vers le noyau
li t0, 0x80000000
jr t0
```

**Adresses matériel utilisées** :
- `UART_BASE = 0x10000000` : adresse de la UART sur QEMU virt
- `UART_TX_REG = 0x00` : registre de transmission
- `UART_LSR_REG = 0x05` : registre d'état de ligne

**Fonction uart_print(a0)** :
- Boucle sur chaque caractère de la chaîne pointée par `a0`
- Vérifie que le registre de transmission est vide (`UART_LSR_THRE`)
- Écrit le caractère dans `UART_TX_REG`
- S'arrête quand elle rencontre le caractère nul (`\0`)

### 2.2 Fichier : boot/boot.ld

**Description** : Script de linkage du bootloader

**Sections** :
- `.text` : code exécutable
- `.rodata` : données en lecture seule (la chaîne "BOOTLOADER OK")
- `.data` : données initialisées
- `.bss` : données non initialisées (la pile de 16 KB)

**Adresse de chargement** : `0x1000` (reset vector sur QEMU virt)

```
. = 0x1000;
.text : { *(.text) }
...
```

### 2.3 Modifications du Makefile

**Ajouts** :

1. **Variables de compilation du bootloader** :
   ```makefile
   BOOT_OBJS = boot/boot.o
   BOOT = boot/boot.elf
   BOOT_BIN = boot/boot.bin
   KERNEL_BIN = kernel/kernel.bin
   ```

2. **Cibles de compilation** :
   ```makefile
   $(BOOT): $(BOOT_OBJS)
       $(LD) -T boot/boot.ld -o $@ $^
   
   $(BOOT_BIN): $(BOOT)
       $(CROSS_COMPILE)objcopy -O binary $< $@
   ```

3. **Cible `make qemu`** :
   ```makefile
   qemu: $(KERNEL) $(BOOT_BIN) $(KERNEL_BIN)
       $(QEMU) -machine virt -bios none \
           -device loader,addr=0x1000,file=$(BOOT_BIN),force-raw=true \
           -device loader,addr=0x80000000,file=$(KERNEL_BIN),force-raw=true \
           -nographic
   ```

**Cible `make debug`** :
   ```makefile
   debug: $(KERNEL) $(BOOT_BIN) $(KERNEL_BIN)
       $(QEMU) -machine virt -bios none \
           -device loader,addr=0x1000,file=$(BOOT_BIN),force-raw=true \
           -device loader,addr=0x80000000,file=$(KERNEL_BIN),force-raw=true \
           -nographic -s -S
   ```

---

## 3. Configuration QEMU

### Approche finale

**Problème initial** :
- QEMU refusait de charger deux fichiers ELF ou des formats mixtes à cause de conflits d'adresses
- Les régions ROM interne de QEMU se chevauchaient avec nos sections

**Solution adoptée** :
- Convertir les exécutables ELF en binaires bruts (`.bin`)
- Utiliser le périphérique generic loader de QEMU (`-device loader`)
- Charger le bootloader au-dessus de la ROM de reset (`0x1000`)
- Charger le noyau à son adresse standard (`0x80000000`)

**Commande QEMU finale** :
```bash
qemu-system-riscv64 -machine virt -bios none \
    -device loader,addr=0x1000,file=boot/boot.bin,force-raw=true \
    -device loader,addr=0x80000000,file=kernel/kernel.bin,force-raw=true \
    -nographic
```

**Paramètres** :
- `-machine virt` : machine QEMU virtuelle RISC-V
- `-bios none` : pas de BIOS par défaut
- `-device loader` : charger un fichier à une adresse donnée
- `addr=0x1000` : adresse du reset vector pour le bootloader
- `addr=0x80000000` : adresse du noyau
- `force-raw=true` : traiter les fichiers `.bin` comme binaires bruts
- `-nographic` : pas d'interface graphique

---

## 4. Flux de démarrage

### Séquence complète

```
1. CPU Reset
   └─> PC = 0x1000 (reset vector)

2. Bootloader (boot/boot.S)
   ├─> Désactive interruptions
   ├─> Initialise pile
   ├─> Affiche "BOOTLOADER OK" sur UART
   └─> Saute à 0x80000000

3. Noyau Lithium (kernel/entry.S + kernel/main.c)
   ├─> entry.S : setup basique CPU, pile, appel kernel_main
   └─> kernel_main : init UART, mémoire, processus, scheduler
        ├─> uart_init()
        ├─> physmem_init()
        ├─> paging_init()
        ├─> trap_init()
        ├─> fs_init()
        ├─> proc_init()
        └─> scheduler()

4. Ordonnanceur
   └─> Exécution des processus utilisateur
```

### Logs de sortie observés

```
BOOTLOADER OK                           ← Message du bootloader
Lithium Kernel starting...               ← Noyau commence
========================================
=== PHYS MEMORY DEBUG ===
sizeof(uint64) = 8
PAGE_SIZE = 4096
PHYS_MEM_SIZE = 134217728
NUM_PAGES = 32768
KERNEL_BASE_ADDR = 0x80000000
&_end = 0x8002115a
========================
kernel_size = 135514 bytes
first_free_page = 34
physmem_init: 32768 pages total, 32734 pages free
paging_init: initializing SV39 page tables
paging_init: kernel page table activated
Allocated page at: 0x80064000
Allocated page at: 0x80065000
Freed page at: 0x80064000
Free pages: 32667

Memory management initialized successfully!

--- Bloc 3: Process Management ---
proc_init: initializing process subsystem
proc_init: init process PID=1

--- Test: fork() ---
fork: processus enfant 2 cree depuis parent 1
Parent: created child PID=2
Parent: child exited with status 0

Bloc 3: SUCCESS!
Starting scheduler...
```

**Interprétation** :
✅ Bootloader détecté et exécuté  
✅ Noyau démarre correctement  
✅ Mémoire physique initialisée  
✅ Tables de pages configurées  
✅ Gestion des processus opérationnelle

---

## 5. Compilations et édition de lien

### Chaîne de compilation pour le bootloader

```bash
# Compilation (assembleur → objet)
riscv64-unknown-elf-gcc -Wall -Werror -O0 \
    -mcmodel=medany -ffreestanding -fno-common -nostdlib \
    -mno-relax -I include -c -o boot/boot.o boot/boot.S

# Édition de lien
riscv64-unknown-elf-ld -T boot/boot.ld -o boot/boot.elf boot/boot.o

# Conversion en binaire brut
riscv64-unknown-elf-objcopy -O binary boot/boot.elf boot/boot.bin
```

### Chaîne de compilation du noyau (inchangée)

```bash
# Compilation des fichiers C et S du noyau
riscv64-unknown-elf-gcc ... -c kernel/entry.S kernel/main.c ...

# Édition de lien
riscv64-unknown-elf-ld -T kernel/kernel.ld -o kernel/kernel.elf ...

# Conversion en binaire brut
riscv64-unknown-elf-objcopy -O binary kernel/kernel.elf kernel/kernel.bin
```

---

## 6. Tests et résultats

### Test 1 : Boot sans crash ✅

**Commande** :
```bash
make clean && make && timeout 3s make qemu
```

**Résultat** :
- Bootloader affiche son message
- Noyau se charge et initialise les sous-systèmes
- Pas d'erreur de segmentation ou de page fault
- Ordonnanceur démarre correctement

### Test 2 : Gestion des processus ✅

- `proc_init()` crée le processus init (PID=1)
- Fork de processus enfants fonctionne
- Allocation/libération de pages fonctionne

### Test 3 : Séquence de mémoire ✅

- Allocation de pages physiques
- Tests de libération
- Compteur de pages libres correct

---

## 7. Améliorations possibles

### Court terme (bootloader amélioré)

1. **Charger le noyau depuis le disque** :
   - Lire le superbloc du disque
   - Parser le système de fichiers
   - Charger dynamiquement le noyau en mémoire

2. **Passer des arguments au noyau** :
   - Device Tree blob (DTB)
   - Ligne de commande
   - Adresse du disque racine

3. **Diagnostic au boot** :
   - Afficher les caractéristiques du CPU (MXLEN, ISA)
   - Vérifier la quantité de RAM
   - Tester la mémoire

### Moyen terme (firmware plus robuste)

1. **OpenSBI** : Intégrer le firmware standard RISC-V
2. **UEFI** : Supporter UEFI pour une vraie machine
3. **Gestion des erreurs** : Gestion des paniques en mode firmware

---

## 8. Fichiers créés/modifiés

### Créés

- `boot/boot.S` : Bootloader RISC-V 68 lignes
- `boot/boot.ld` : Script de linkage du bootloader
- `BOOTLOADER_REPORT.md` : Ce rapport

### Modifiés

- `Makefile` : Ajout des cibles boot, bin, qemu, debug

### Structures de répertoires

```
boot/
├── boot.S       ← Bootloader en assembleur
├── boot.ld      ← Linker script
├── boot.o       ← Objet compilé (généré)
├── boot.elf     ← Exécutable ELF (généré)
└── boot.bin     ← Binaire brut pour QEMU (généré)
```

---

## 9. Utilisation

### Build complet

```bash
make clean   # Nettoyer les anciens fichiers
make         # Compiler bootloader + noyau
```

### Exécution dans QEMU

```bash
make qemu    # Lancer le noyau avec bootloader
```

### Debug avec GDB

```bash
# Terminal 1: démarrer QEMU en attente
make debug

# Terminal 2: connecter GDB
gdb-multiarch kernel/kernel.elf
(gdb) target remote :1234
(gdb) b _start
(gdb) c         # Continue jusqu'au breakpoint
```

---

## 10. Conclusion

### Accomplissements

✅ Bootloader minimal et fonctionnel implémenté  
✅ Démarrage à deux étapes : bootloader → noyau  
✅ Diagnostic du bootloader sur UART  
✅ Noyau démarre correctement après le bootloader  
✅ Build automatisé avec Makefile  
✅ Tests en QEMU réussis  

### Architecture actuelle

```
Lithium Kernel Boot Architecture
================================

QEMU Setup
  ├─ Charge boot/boot.bin à 0x1000 (reset vector)
  └─ Charge kernel/kernel.bin à 0x80000000

CPU Reset → PC = 0x1000
  ↓
boot/boot.S (_start)
  • Désactive interruptions
  • Init pile
  • Affiche "BOOTLOADER OK"
  • Saute à 0x80000000
  ↓
kernel/entry.S (_start)
  • Init stack du kernel
  • Appelle kernel_main()
  ↓
kernel/main.c (kernel_main)
  • uart_init()
  • physmem_init()
  • paging_init()
  • trap_init()
  • fs_init()
  • proc_init()
  • scheduler()
  ↓
Scheduler → Processus utilisateur
```

### Prochaines étapes

Le bootloader peut maintenant être étendu pour :
- Lire et charger le noyau depuis le disque
- Implémenter un Device Tree
- Ajouter une vraie interface de boot (menu, arguments)
- Supporter les vraies machines RISC-V (SiFive, StarFive, etc.)

---

**Fin du rapport**

*Rapport généré le 25 juillet 2026 pour le projet Lithium Kernel*
