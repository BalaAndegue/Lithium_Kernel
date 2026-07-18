#ifndef KERNEL_IO_KPANIC_H
#define KERNEL_IO_KPANIC_H

// Kernel panic - halts the kernel and displays an error message
// This function NEVER returns - it enters an infinite loop
void panic(const char *fmt, ...);

// Halt the CPU - infinite loop (called by panic)
void halt(void);

// Set the panic source information (file, line)
// These are typically set via macros, not called directly
void panic_set_context(const char *file, int line);

#endif
