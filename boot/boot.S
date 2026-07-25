# boot.S - Bootloader minimal pour RISC-V QEMU virt.
# Ce bootloader initialise la pile, affiche un message sur UART,
# puis bloque.

.equ UART_BASE, 0x10000000
.equ UART_TX_REG, 0x00
.equ UART_LSR_REG, 0x05
.equ UART_LSR_THRE, 1 << 5

.section .text
.globl _start
_start:
    # Désactiver les interruptions machine
    csrw mstatus, zero

    # Initialiser la pile
    la sp, stack_top

    # Afficher un message simple sur l'UART
    la a0, boot_msg
    call uart_print

    # Sauter vers le noyau chargé à l'adresse 0x80000000
    li t0, 0x80000000
    jr t0

    # Boucle infinie de secours
hang:
    wfi
    j hang

# Fonction uart_print(char *s)
# a0 = adresse de la chaîne
uart_print:
    lbu a1, 0(a0)
    beqz a1, uart_done

wait_tx_ready:
    li t0, UART_BASE + UART_LSR_REG
    lbu t1, 0(t0)
    andi t1, t1, UART_LSR_THRE
    beqz t1, wait_tx_ready

    li t0, UART_BASE + UART_TX_REG
    sb a1, 0(t0)

    addi a0, a0, 1
    j uart_print

uart_done:
    ret

.section .rodata
.align 4
boot_msg:
    .ascii "BOOTLOADER OK\n\0"

.section .bss
.align 16
.space 4096 * 4
stack_top:
