#include "kernel/io/debug.h"

#ifdef PANIC_TEST

void panic_test(void)
{
    printf("\n--- Test: panic() ---\n");
    PANIC_FMT("panic test %d", 42);
}

#endif
