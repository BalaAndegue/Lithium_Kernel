#ifndef KERNEL_IO_DEBUG_H
#define KERNEL_IO_DEBUG_H

#include "kernel/io/kpanic.h"
#include "kernel/io/printk.h"

// Debug macro to trigger panic with file and line context
#define PANIC(msg) \
    do { \
        panic_set_context(__FILE__, __LINE__); \
        panic(msg); \
    } while(0)

// Debug macro with formatted arguments
#define PANIC_FMT(fmt, ...) \
    do { \
        panic_set_context(__FILE__, __LINE__); \
        printk("\n====== PANIC at %s:%d ======\n", __FILE__, __LINE__); \
        printk("ERROR: " fmt "\n", __VA_ARGS__); \
        printk("====== System halted ======\n"); \
        halt(); \
    } while(0)

// Assert macro - panic if condition is false
#define ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            panic_set_context(__FILE__, __LINE__); \
            panic("ASSERTION FAILED: " msg); \
        } \
    } while(0)

// Assert macro with format string
#define ASSERT_FMT(cond, fmt, ...) \
    do { \
        if (!(cond)) { \
            panic_set_context(__FILE__, __LINE__); \
            printk("\n====== ASSERTION FAILED at %s:%d ======\n", __FILE__, __LINE__); \
            printk("CHECK: " fmt "\n", __VA_ARGS__); \
            printk("====== System halted ======\n"); \
            halt(); \
        } \
    } while(0)

// Conditional debug print (only prints if DEBUG is defined)
#ifdef DEBUG
    #define DEBUG_PRINT(fmt, ...) printk("[DEBUG] " fmt "\n", __VA_ARGS__)
    #define DEBUG_WARN(msg) printk("[WARN] %s at %s:%d\n", msg, __FILE__, __LINE__)
#else
    #define DEBUG_PRINT(fmt, ...) do {} while(0)
    #define DEBUG_WARN(msg) do {} while(0)
#endif

// Error print (always enabled)
#define ERROR_PRINT(fmt, ...) printk("[ERROR] " fmt " (%s:%d)\n", __VA_ARGS__, __FILE__, __LINE__)

// Unimplemented feature
#define UNIMPLEMENTED() PANIC("Not implemented yet!")

// Unreachable code path
#define UNREACHABLE() PANIC("Reached unreachable code!")

#endif
