#include "kernel/io/kpanic.h"
#include "kernel/io/uart.h"
#include <stdarg.h>

// Forward declare printk to avoid circular dependency
extern int printk(const char *fmt, ...);

// Global panic context
static const char *panic_file = "?";
static int panic_line = 0;

// Set panic source information
void panic_set_context(const char *file, int line)
{
    panic_file = file;
    panic_line = line;
}

// Halt the CPU - infinite loop (no way back)
void halt(void)
{
    while (1) {
        // Prevent compiler from optimizing away this loop
        __asm__ volatile("wfi");  // RISC-V wait for interrupt
    }
}

// Kernel panic - print error message and halt
// This is a simple panic that just prints a message
void panic(const char *fmt, ...)
{
    // Disable interrupts to prevent any interruption
    // TODO: Call interrupt disable function when available
    
    // Print dramatic error banner
    uart_puts("\n");
    uart_puts("====================================================\n");
    uart_puts("                   KERNEL PANIC!\n");
    uart_puts("====================================================\n");
    
    // Print source location
    if (panic_file && panic_line > 0) {
        uart_puts("Location: ");
        uart_puts((char*)panic_file);  // Cast away const for uart_puts
        uart_puts(":");
        // Print line number (simple int-to-string)
        char line_str[16];
        int line_copy = panic_line;
        int idx = 0;
        if (line_copy == 0) {
            line_str[0] = '0';
            line_str[1] = '\0';
        } else {
            // Convert to string
            while (line_copy > 0 && idx < 15) {
                line_str[idx++] = '0' + (line_copy % 10);
                line_copy /= 10;
            }
            line_str[idx] = '\0';
            // Reverse it
            for (int i = 0; i < idx / 2; i++) {
                char tmp = line_str[i];
                line_str[i] = line_str[idx - 1 - i];
                line_str[idx - 1 - i] = tmp;
            }
        }
        uart_puts(line_str);
        uart_puts("\n");
    }
    
    // Print error message using printk if available
    if (fmt) {
        uart_puts("Error: ");
        va_list ap;
        va_start(ap, fmt);
        // Note: we can't use printk with va_list easily, so we'll just print the format string
        // A better solution would implement vsnprintf
        uart_puts((char*)fmt);
        va_end(ap);
        uart_puts("\n");
    }
    
    // Print final message
    uart_puts("System halted. Check serial output for details.\n");
    uart_puts("====================================================\n");
    
    // Halt the system
    halt();
}
