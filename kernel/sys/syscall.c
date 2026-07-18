// ===========================================================================
// syscall.c - Dispatcher et implémentation des appels système
// ===========================================================================
//
// Quand un processus exécute l'instruction "ecall", le CPU génère un piège.
// kerneltrap() détecte scause == 8 (ecall depuis user) et appelle syscall().
//
// Convention d'appel RISC-V :
//   a7 = numéro de l'appel système
//   a0, a1, a2... = arguments
//   a0 = valeur de retour (écrite dans le trapframe du processus)
// ===========================================================================

#include <stddef.h>
#include "kernel/types.h"
#include "kernel/sys/syscall.h"
#include "kernel/sys/number.h"
#include "kernel/proc/process.h"
#include "kernel/proc/control.h"
#include "kernel/proc/scheduler.h"
#include "kernel/proc/globals.h"
#include "kernel/io/console.h"

// ---------------------------------------------------------------------------
// Helpers : lire les arguments depuis le trapframe du processus courant
// ---------------------------------------------------------------------------
static uint64 argraw(int n)
{
    struct proc *p = current_process;
    if (p == NULL || p->trapframe == NULL) return 0;

    switch (n) {
    case 0: return p->trapframe->a0;
    case 1: return p->trapframe->a1;
    case 2: return p->trapframe->a2;
    case 3: return p->trapframe->a3;
    case 4: return p->trapframe->a4;
    case 5: return p->trapframe->a5;
    }
    return 0;
}

static int    argint(int n)  { return (int)argraw(n); }
static uint64 arguint(int n) { return argraw(n); }

// ---------------------------------------------------------------------------
// sys_fork - Créer un processus fils
// ---------------------------------------------------------------------------
int sys_fork(void)
{
    return fork();
}

// ---------------------------------------------------------------------------
// sys_exit - Terminer le processus courant
// ---------------------------------------------------------------------------
int sys_exit(void)
{
    int code = argint(0);
    exit(code);
    return 0;
}

// ---------------------------------------------------------------------------
// sys_wait - Attendre la fin d'un fils
// ---------------------------------------------------------------------------
int sys_wait(void)
{
    return wait(0);  // Pass a dummy address for now
}

// ---------------------------------------------------------------------------
// sys_write - Écrire sur un descripteur de fichier
// Seuls fd=1 (stdout) et fd=2 (stderr) sont supportés pour l'instant
// ---------------------------------------------------------------------------
int sys_write(void)
{
    int    fd      = argint(0);
    char  *buf     = (char *)arguint(1);
    int    n       = argint(2);

    if ((fd == 1 || fd == 2) && buf != NULL && n > 0) {
        extern void console_putchar(char c);
        for (int i = 0; i < n; i++)
            console_putchar(buf[i]);
        return n;
    }
    return -1;
}

// ---------------------------------------------------------------------------
// sys_read - Lire depuis un descripteur (stub — nécessite le bloc 5)
// ---------------------------------------------------------------------------
int sys_read(void)
{
    return -1;
}

// ---------------------------------------------------------------------------
// sys_getpid - Retourner le PID du processus courant
// ---------------------------------------------------------------------------
int sys_getpid(void)
{
    struct proc *p = current_process;
    return (p != NULL) ? p->pid : -1;
}

// ---------------------------------------------------------------------------
// sys_sleep - Endormir le processus N ticks
// ---------------------------------------------------------------------------
int sys_sleep(void)
{
    int ticks = argint(0);
    sleep((void *)(uint64)ticks, NULL);
    return 0;
}

// ---------------------------------------------------------------------------
// sys_yield - Céder la main à l'ordonnanceur
// ---------------------------------------------------------------------------
int sys_yield(void)
{
    yield();
    return 0;
}

// ---------------------------------------------------------------------------
// Table de dispatch : numéro d'appel → fonction handler
// ---------------------------------------------------------------------------
typedef int (*syscall_fn)(void);

static syscall_fn syscall_table[NSYSCALLS] = {
    NULL,         // 0 — réservé
    sys_fork,     // 1 — SYS_fork
    sys_exit,     // 2 — SYS_exit
    sys_wait,     // 3 — SYS_wait
    sys_read,     // 4 — SYS_read
    sys_write,    // 5 — SYS_write
    sys_getpid,   // 6 — SYS_getpid
    sys_sleep,    // 7 — SYS_sleep
    sys_yield,    // 8 — SYS_yield
};

// ---------------------------------------------------------------------------
// syscall - Point d'entrée depuis kerneltrap
// ---------------------------------------------------------------------------
void syscall(void)
{
    struct proc *p = current_process;
    if (p == NULL || p->trapframe == NULL) return;

    int num = (int)p->trapframe->a7;

    if (num > 0 && num < NSYSCALLS && syscall_table[num] != NULL) {
        p->trapframe->a0 = (uint64)syscall_table[num]();
    } else {
        printf("syscall: numéro inconnu %d (pid=%d)\n", num, p->pid);
        p->trapframe->a0 = (uint64)-1;
    }
}
