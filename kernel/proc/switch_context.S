# ===========================================================================
# switch_context.S - Changement de contexte entre deux processus
# ===========================================================================
# Sauvegarde les registres du processus actuel et restaure ceux du nouveau
# 
# void context_switch(struct context *old, struct context *new)
# a0 = adresse du contexte à sauvegarder (old_context)
# a1 = adresse du contexte à restaurer (new_context)
# ===========================================================================

.section .text
.globl context_switch
context_switch:
    # ========== Sauvegarder les registres du VIEUX contexte dans (a0) ==========
    # a0 pointe sur la structure context du processus précédent
    
    sd ra, 0(a0)      # Sauvegarder l'adresse de retour
    sd sp, 8(a0)      # Sauvegarder le pointeur de pile
    sd s0, 16(a0)     # Sauvegarder les registres préservés s0-s11
    sd s1, 24(a0)
    sd s2, 32(a0)
    sd s3, 40(a0)
    sd s4, 48(a0)
    sd s5, 56(a0)
    sd s6, 64(a0)
    sd s7, 72(a0)
    sd s8, 80(a0)
    sd s9, 88(a0)
    sd s10, 96(a0)
    sd s11, 104(a0)
    
    # ========== Restaurer les registres du NOUVEAU contexte depuis (a1) ==========
    # a1 pointe sur la structure context du nouveau processus
    
    ld ra, 0(a1)      # Restaurer l'adresse de retour
    ld sp, 8(a1)      # Restaurer le pointeur de pile
    ld s0, 16(a1)     # Restaurer les registres préservés s0-s11
    ld s1, 24(a1)
    ld s2, 32(a1)
    ld s3, 40(a1)
    ld s4, 48(a1)
    ld s5, 56(a1)
    ld s6, 64(a1)
    ld s7, 72(a1)
    ld s8, 80(a1)
    ld s9, 88(a1)
    ld s10, 96(a1)
    ld s11, 104(a1)
    
    # Retourner au nouveau processus
    ret
