// ===========================================================================
// syscall.h (user) - Interface des appels système côté espace utilisateur
// ===========================================================================
// Ces fonctions émettent l'instruction "ecall" avec le bon numéro dans a7.
// Le kernel reçoit le piège, dispatch vers syscall.c et retourne dans a0.
// ===========================================================================

#ifndef USER_SYSCALL_H
#define USER_SYSCALL_H

#include "kernel/types.h"
#include "kernel/sys/number.h"

// Appel système générique : charge num dans a7, args dans a0-a5, ecall
static inline int64 syscall0(int num)
{
    int64 ret;
    asm volatile(
        "mv a7, %1\n"
        "ecall\n"
        "mv %0, a0\n"
        : "=r"(ret)
        : "r"(num)
        : "a0", "a7"
    );
    return ret;
}

static inline int64 syscall1(int num, uint64 a0)
{
    int64 ret;
    asm volatile(
        "mv a0, %2\n"
        "mv a7, %1\n"
        "ecall\n"
        "mv %0, a0\n"
        : "=r"(ret)
        : "r"(num), "r"(a0)
        : "a0", "a7"
    );
    return ret;
}

static inline int64 syscall3(int num, uint64 a0, uint64 a1, uint64 a2)
{
    int64 ret;
    asm volatile(
        "mv a0, %2\n"
        "mv a1, %3\n"
        "mv a2, %4\n"
        "mv a7, %1\n"
        "ecall\n"
        "mv %0, a0\n"
        : "=r"(ret)
        : "r"(num), "r"(a0), "r"(a1), "r"(a2)
        : "a0", "a1", "a2", "a7"
    );
    return ret;
}

// ---------------------------------------------------------------------------
// Wrappers nommés — API publique pour les programmes utilisateur
// ---------------------------------------------------------------------------
static inline int  u_fork(void)              { return (int)syscall0(SYS_fork); }
static inline void u_exit(int code)          { syscall1(SYS_exit, (uint64)code); }
static inline int  u_wait(void)              { return (int)syscall0(SYS_wait); }
static inline int  u_getpid(void)            { return (int)syscall0(SYS_getpid); }
static inline void u_yield(void)             { syscall0(SYS_yield); }
static inline void u_sleep(int ticks)        { syscall1(SYS_sleep, (uint64)ticks); }

static inline int u_write(int fd, const void *buf, int n) {
    return (int)syscall3(SYS_write, (uint64)fd, (uint64)buf, (uint64)n);
}

static inline int u_read(int fd, void *buf, int n) {
    return (int)syscall3(SYS_read, (uint64)fd, (uint64)buf, (uint64)n);
}

#endif
