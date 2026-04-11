/*
 * ce fichier dit au linker ou placer chaque code en memoire.
 * pour RISC-V QEMU virt , le noyau est charge a l'address 0x80000000.
*/

/* architecture cible : RISC-V 64 bits */

OUTPUT_ARCH(riscv)

/* Point d'entree : symbole _start defini dans entry.Sauter

ENTRY(_start)

/* Adress de base ou le noyau est charge en memoire */

BASE_ADDRESS = 0x80000000;

SECTIONS 
{
    /* section .text : code executable */

    . = BASE_ADDRESS ;
    .text : {
        *(.text)          /* Toutes les sections .text de tous les fichiers */

    }
    /* section .rodata : donnees en lecture seule (constantes , chaines) */
    .rodata :{
        *(.rodata)
    }
    /* section.data donnees Initialisees (variables globales avec valeur) */
    .data : {

        *(.data)

    }
    /* section .bss : donnees non initialisees (initialisees a zero) */
    .bss : {
        *(.bss)
    }
    /* fin du noyau par l'allocateur memoire */
    _end = .;

}