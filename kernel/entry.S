#ce fichier est la premiere instruction execute par le processeur

#objetifs :
#   1. Desactiver les interruptions 
#   2. Initialiser le pointeur 
#   3. Sauter vers le code C(kernel_main)

.section .text                          # section code (executable)
.globl _start                           #  rendre _start visible par le linker

_start:


    # 1. Desactiver toutes les interruptions

    # mstatuts est le registre d'etat machine (Machine statuts Register)
    # en ecrivant 0 , on desactive les interruptions au niveau machine

    csrw  mstatus , zero 

    # 2. Initialiser le pointeur de pile (stack pointer)

    # la pile est definie plus bas dans la section .bss
    # stack_top pointe vers le haut de la pile (la pile croit vers le bas)

    la sp, stack_top

    # 3. sauter vers le code C

    # call sauvegarde l'adresse de retour dans ra( return address)
    # et saute vers le kernel_main

    call kernel_main 

    # 4. boucle infinie( de devrait jamais etre atteint)

hang:

    wfi                                # wait for interrupt (reduit la consommation)
    j hang                             # jump vers le hang ( car c'est une boucle infinie)

# section .bss - memoire non Initialisee (Initialisee a zero)


.section .bss
.align 16                              # alignement sur 16 octets (obligatoire pour la pile)

# reservation de 4 pages (16ko) pour la pile noyau

.space 4096 * 4
stack_top :                   # etiquette pointant vers le haut de la pile 