// ===========================================================================
// syscall.h - Interface du dispatcher d'appels système
// ===========================================================================

#ifndef KERNEL_SYS_SYSCALL_H
#define KERNEL_SYS_SYSCALL_H

// Point d'entrée appelé par kerneltrap quand scause == SCAUSE_ECALL_USER
void syscall(void);

// Fonctions internes — une par appel système
int sys_fork(void);
int sys_exit(void);
int sys_wait(void);
int sys_read(void);
int sys_write(void);
int sys_getpid(void);
int sys_sleep(void);
int sys_yield(void);

#endif
