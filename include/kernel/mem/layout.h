// ce fichier permet d'organiser la memoir et definit ou se trouve chaque section de memoire .


#ifndef KERNEL_MEM_LAYOUT_H
#define KERNEL_MEM_LAYOUT_H

#include "kernel/types.h"


// ===========================================================================
// addresses memoire importantes pour le kernel
// ===========================================================================

// Addresse de chargement du kernel dans la memoire physique (QEMU RISC-V virt charge le kernel à 0x80000000 )
#define KERNEL_BASE_ADDR 0x80000000UL

// Taille maximale de la memoire physique (128 Mo pour QEMU RISC-V virt)

#define PHYS_MEM_SIZE (128 * 1024 * 1024UL)

//taille d'une page (4ko)
#define PAGE_SIZE 4096UL

// Nombre de pages 
#define NUM_PAGES (PHYS_MEM_SIZE / PAGE_SIZE)

//Addresses virtuelles (pour plus tard)

// Noyau en high-half : on décale l'espace virtuel par rapport à l'espace physique.
// La mémoire physique commence à 0x80000000 et est exposée en virtuel à 0xC0000000.
#define KERNEL_VIRT_BASE 0xC0000000UL
#define PHYS_TO_VIRT_OFFSET (KERNEL_VIRT_BASE - KERNEL_BASE_ADDR)

//Conversion des addresses physiques en virtuelles et inversement
#define phys_to_virt(phys) ((void *)((uint64)(phys) + PHYS_TO_VIRT_OFFSET))
#define virt_to_phys(virt) ((uint64)(virt) - PHYS_TO_VIRT_OFFSET)

#endif 