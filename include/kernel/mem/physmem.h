// Allocateur de pages physiques pour le kernel
// Ce module gère l'allocation et la libération de pages de mémoire physique ( 4 KO chacune)
// utilise une bitmap pour suivre l'état de chaque page (libre ou allouée)

#ifndef KERNEL_MEM_PHYSMEM_H
#define KERNEL_MEM_PHYSMEM_H

#include "kernel/types.h"
#include "kernel/mem/layout.h"

//Initialisation
// Inittiaise l'allocateur de la memoire physique
// doit etre appelé une fois au démarrage du kernel pour configurer la bitmap et marquer les pages utilisées par le kernel
void physmem_init(void);

//Allocation/Libération
// Alloue une page de mémoire physique et retourne son adresse physique
// Retourne 0 si aucune page n'est disponible

uint64 physmem_alloc_page(void);

// Libère une page de mémoire physique donnée par son adresse physique
// Paramètre :
//   phys_addr - Adresse physique de la page à libérer (doit être alignée sur une frontière de page)
void free_physical_page(uint64 phys_addr);

// statistiques
// Retourne le nombre de pages physiques actuellement allouées
uint64 get_free_pages_count(void);

#endif