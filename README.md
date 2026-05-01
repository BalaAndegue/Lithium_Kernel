
# Lithium Kernel

A minimal UNIX-like kernel for RISC-V architecture, developed as part of an operating systems course.  
Inspired by xv6, this implementation focuses on educational clarity with modular design and comprehensive documentation.

## Academic Supervision

**Professor Alain Tchana** - Process and memory management modules  
**Professor Daniel Hagimont** - System architecture and design

## Educational Objectives

- Understand operating system fundamentals through hands-on implementation
- Master RISC-V assembly and low-level programming
- Implement core OS abstractions: processes, memory management, scheduling
- Develop professional software engineering practices in kernel development
- Document implementation decisions and design choices

## Architecture

| Component           | Specification                          |
|---------------------|----------------------------------------|
| ISA                 | RISC-V 64-bit (RV64IMAC)               |
| Memory Model        | SV39 (3-level page tables, 4KB pages)  |
| Platform            | QEMU `virt` machine                    |
| Bootloader          | None (direct kernel boot via `-kernel`)|
| Languages           | C17, RISC-V assembly                   |
| Toolchain           | `riscv64-unknown-elf-gcc`              |
| Debugging           | GDB + QEMU (port 1234)                 |

## Project Structure

```
lithium-kernel/
├── Makefile                    # Build automation
├── README.md                   # Project documentation
├── TODO.md                     # Development roadmap
├── RAPPORT_BLOC2_MEMOIRE.md    # Memory management report
├── .gitignore
├── .gdbinit
│
├── include/
│   ├── kernel/
│   │   ├── fs/                 # File system headers (block, buffer, file, inode, log, pipe, virtio)
│   │   ├── io/                 # I/O headers (console, debug, uart, printk, trace, kpanic)
│   │   ├── mem/                # Memory headers (kmalloc, layout, operations, paging, physmem, spinlock, virtual_memory)
│   │   ├── proc/               # Process headers (control, elf, globals, process, scheduler)
│   │   ├── sys/                # System call headers (number, syscall)
│   │   ├── trap/               # Trap headers (plic, trap)
│   │   ├── param.h             # Kernel parameters
│   │   └── types.h             # Type definitions
│   ├── make_fs/                # Disk image builder headers
│   ├── riscv/                  # RISC-V specific headers (intrinsics, defs, utils, virtual_memory)
│   └── user/                   # User space headers (fnctl, limits, syscall, ulibc)
│
├── kernel/
│   ├── entry.S                 # Boot entry point (assembly)
│   ├── kernel.ld               # Linker script
│   ├── main.c                  # kernel_main() – initialization
│   ├── setup.c                 # Low-level CPU setup
│   ├── fs/                     # File system implementation (mostly stubs)
│   │   ├── block.c             # Block device interface
│   │   ├── buffer.c            # Buffer cache
│   │   ├── file.c              # File operations
│   │   ├── file_system.c       # File system core
│   │   ├── inode.c             # Inode management
│   │   ├── log.c               # File system logging
│   │   ├── pipe.c              # Pipe implementation
│   │   ├── sleeplock.c         # Sleep locks
│   │   └── virtio_disk.c       # VirtIO disk driver
│   ├── io/                     # Drivers
│   │   ├── console.c           # Console I/O (implemented)
│   │   ├── debug.c             # Debug utilities
│   │   ├── kpanic.c            # Kernel panic handling
│   │   ├── printk.c            # Formatted printing
│   │   ├── trace.c             # Execution tracing
│   │   └── uart.c              # UART driver (implemented)
│   ├── mem/                    # Memory management
│   │   ├── kmalloc.c           # Kernel memory allocator
│   │   ├── operations.c        # Memory operations
│   │   ├── paging.c            # Page table management (implemented)
│   │   ├── physmem.c           # Physical memory allocator (implemented)
│   │   ├── spinlock.c          # Spin locks
│   │   ├── virtual_memory.c    # Virtual memory utilities
│   │   └── layout.h            # Memory layout definitions
│   ├── proc/                   # Process management (Bloc 3 - implemented)
│   │   ├── control.c           # Process control (fork, exit, wait, sleep, wakeup)
│   │   ├── exec.c              # Program execution (stub)
│   │   ├── process.c           # Process lifecycle management
│   │   ├── scheduler.c         # Round-robin scheduler
│   │   └── switch_context.S    # Context switching (assembly)
│   ├── sys/                    # System calls (stubs)
│   └── trap/                   # Exception handling (stubs)
│
├── make_fs/                    # Disk image build directory
├── tools/                      # Host utilities
└── user/                       # User programs (stubs)
    ├── progs/                  # User binaries
    └── ulibc/                  # User libc
```

## Quick Start

### 1. Install Toolchain

**Ubuntu / Debian**
```bash
sudo apt install gcc-riscv64-unknown-elf qemu-system-misc gdb-multiarch make
```

**Arch Linux**
```bash
sudo pacman -S riscv64-elf-gcc qemu-system-riscv gdb-multiarch make
```

**macOS (Homebrew)**
```bash
brew install riscv-tools qemu gdb
```

### 2. Build

```bash
make clean
make
```

### 3. Run

```bash
make qemu
```

Expected output (Bloc 1 + Bloc 2 + Bloc 3):
```
Lithium Kernel starting...
uart initialized.
mem_init: 128 MB available
proc_init: initializing process subsystem
proc_init: init process PID=1
scheduler: entering main loop
```

### 4. Debug

```bash
# Terminal 1
make debug

# Terminal 2
gdb-multiarch kernel/kernel.elf
(gdb) target remote :1234
(gdb) break main.c:42
(gdb) continue
```

## Build Commands

| Command       | Description                                      |
|---------------|--------------------------------------------------|
| `make`        | Build kernel image (`kernel/kernel.elf`)         |
| `make clean`  | Remove all object files and kernel image         |
| `make qemu`   | Launch QEMU with the kernel                      |
| `make debug`  | Launch QEMU in debug mode (listening on port 1234)|
| `make fs`     | Build disk image (`make_fs/fs.img`) - Not implemented |
| `make all`    | Build kernel + disk image - Not implemented      |

## Development Progress

### Completed Blocks
- **Bloc 1**: Boot + Console ✅ (Semaine 1)
- **Bloc 2**: Memory Management ✅ (Semaine 2) - Report: `RAPPORT_BLOC2_MEMOIRE.md`
- **Bloc 3**: Process Management ✅ (Semaine 3-4) - 13 commits pushed

### Current Status
- **Bloc 4**: System calls + Traps (Semaine 5) - In progress
- **Bloc 5**: File System + Drivers (Semaine 6-7) - Planned
- **Bloc 6**: User Space (Semaine 8) - Planned

### Documentation
- `RAPPORT_BLOC2_MEMOIRE.md`: Detailed memory management implementation report
- `TODO.md`: Development roadmap and task breakdown
- All code includes French comments for clarity

## Module Breakdown

| Module     | Location               | Status | Description                                                                 |
|------------|------------------------|--------|-----------------------------------------------------------------------------|
| **Boot**   | `entry.S`, `setup.c`   | ✅ Implémenté | CPU initialization, stack setup, jump to `kernel_main()`                    |
| **Memory** | `mem/`                 | 🟡 Partiel | Physical page allocator (bitmap), SV39 page tables (paging.c, physmem.c implémentés) |
| **Proc**   | `proc/`                | ✅ Implémenté | Process lifecycle (`fork`, `exit`, `wait`), round-robin scheduler (Bloc 3 terminé) |
| **Trap**   | `trap/`                | ❌ Stub | Exception handling, interrupt routing, timer interrupts                     |
| **Syscall**| `sys/`                 | ❌ Stub | User→kernel interface (`write`, `read`, `fork`, `exec`, `exit`, `wait`)      |
| **Driver** | `io/`                  | 🟡 Partiel | Serial output (UART implémenté), console I/O (console.c implémenté)         |
| **FS**     | `fs/`                  | ❌ Stub | Simple UNIX file system (inodes, directories, block cache, VirtIO disk)     |
| **User**   | `user/`                | ❌ Stub | First user process (`init`), system call wrappers                           |

## Key Data Structures

### Process Management (Implemented in Bloc 3)

```c
// Process states
enum proc_state {
    PROC_UNUSED = 0,    // Not allocated
    PROC_USED,          // Allocated but not ready
    PROC_SLEEPING,      // Waiting for event
    PROC_RUNNABLE,      // Ready to run
    PROC_RUNNING,       // Currently executing
    PROC_ZOMBIE         // Terminated, waiting for parent
};

// Process structure
struct proc {
    enum proc_state state;      // Current state
    int pid;                    // Process ID
    struct proc *parent;        // Parent process
    void *pagetable;            // Page table
    uint64 sz;                  // Memory size
    uint64 kstack;              // Kernel stack
    struct trapframe *trapframe; // Saved registers
    struct context context;     // CPU context for switching
    int exit_code;              // Exit status
    char name[32];              // Process name
};

// CPU context (preserved registers)
struct context {
    uint64 ra, sp, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11;
};
```

### Memory Management (Implemented in Bloc 2)

```c
// Page Table Entry (SV39 format)
typedef uint64 pte_t;
#define PTE_V (1 << 0)        // Valid
#define PTE_R (1 << 1)        // Read
#define PTE_W (1 << 2)        // Write
#define PTE_X (1 << 3)        // Execute
#define PTE_U (1 << 4)        // User accessible
#define PTE_A (1 << 6)        // Accessed
#define PTE_D (1 << 7)        // Dirty
```

### ELF Binary Format (Headers for Bloc 4)

```c
// ELF file header (64-bit)
struct elfhdr {
    uint32 magic;      // 0x464C457F ("\x7FELF")
    uint16 type;       // Executable, object, etc.
    uint16 machine;    // RISC-V = 0xF3
    uint64 entry;      // Entry point address
    uint64 phoff;      // Program header offset
    // ... other fields
};

// Program header (loadable segments)
struct proghdr {
    uint32 type;       // Segment type (LOAD = 1)
    uint32 flags;      // Permissions (R/W/X)
    uint64 vaddr;      // Virtual address
    uint64 filesz;     // Size in file
    uint64 memsz;      // Size in memory
    // ... other fields
};
```

## Coding Conventions

| Rule                          | Rationale                                     |
|-------------------------------|-----------------------------------------------|
| 4-space indentation, no tabs  | Consistency across editors                    |
| `snake_case` for functions    | UNIX kernel style                             |
| `UPPER_CASE` for macros       | Standard C practice                           |
| `/* comment */` for multi-line| C89 compatibility                             |
| `// comment` for single-line  | Permitted in C17                              |
| French comments in code       | Educational project requirement               |
| No dynamic allocation after boot | Deterministic memory usage                 |
| Every `alloc` has matching `free` | Prevent memory leaks                      |
| Validate all user pointers    | Security (kernel must not crash on bad input) |

## Version Control

The project follows a structured commit approach with logical groupings:

### Recent Commits (Bloc 3 - Process Management)
- **13 commits** pushed to GitHub with detailed commit messages
- Each commit represents a logical unit of functionality
- French comments added throughout the codebase
- Professional commit messages following conventional standards

## Documentation

### Reports
- **`RAPPORT_BLOC2_MEMOIRE.md`**: Comprehensive memory management implementation report
  - Physical memory allocation strategies
  - Page table management (SV39)
  - Virtual memory organization
  - Performance analysis and testing

### Development Roadmap
- **`TODO.md`**: Detailed task breakdown by development blocks
- Weekly progression tracking
- Implementation priorities and dependencies

### Code Documentation
- All functions include French comments explaining their purpose
- Data structures are thoroughly documented
- Implementation decisions are justified in comments
- Educational approach with clear explanations

## Debugging Tips

### Common kernel panics

| Symptom                          | Likely cause                              | Fix                                   |
|----------------------------------|-------------------------------------------|---------------------------------------|
| `scause=0x0000000000000002`      | Illegal instruction                       | Check `ecall`/`ebreak` usage          |
| `scause=0x000000000000000c`      | Page fault (load)                         | Missing mapping in page table         |
| `scause=0x000000000000000d`      | Page fault (store)                        | Write to read-only page               |
| `scause=0x0000000080000005`      | Timer interrupt (normal)                  | Not an error, handle it               |
| QEMU hangs with no output        | UART not initialized or wrong address     | Verify `UART_BASE = 0x10000000`       |
| Infinite reboot loop             | Stack overflow or misaligned `sp`         | Check `entry.S` stack alignment       |

### GDB quick reference

```bash
# In QEMU debug mode (make debug)
# Connect GDB
(gdb) target remote :1234

# Useful commands
(gdb) info registers
(gdb) x/10x $sp
(gdb) bt
(gdb) break kernel/mem/paging.c:42
(gdb) watch *0x80000000
(gdb) continue
(gdb) stepi
(gdb) disas
```

## Makefile Example

```makefile
CROSS_COMPILE = riscv64-unknown-elf-
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
QEMU = qemu-system-riscv64

CFLAGS = -Wall -Werror -O2 -fno-omit-frame-pointer -ggdb -gdwarf-2
CFLAGS += -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib
CFLAGS += -mno-relax -I include

LDFLAGS = -T kernel/kernel.ld -z max-page-size=4096

KERNEL_OBJS = \
	kernel/entry.o \
	kernel/main.o \
	kernel/setup.o \
	kernel/io/uart.o \
	kernel/io/console.o \
	kernel/mem/operations.o \
	kernel/mem/kmalloc.o \
	kernel/mem/spinlock.o \
	kernel/mem/virtual_memory.o \
	kernel/proc/process.o \
	kernel/proc/scheduler.o \
	kernel/proc/control.o \
	kernel/proc/exec.o \
	kernel/proc/switch_context.o \
	kernel/sys/syscall.o

KERNEL = kernel/kernel.elf

all: $(KERNEL)

$(KERNEL): $(KERNEL_OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.S
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(KERNEL_OBJS) $(KERNEL)

qemu: $(KERNEL)
	$(QEMU) -machine virt -bios none -kernel $(KERNEL) -nographic

debug: $(KERNEL)
	$(QEMU) -machine virt -bios none -kernel $(KERNEL) -nographic -s -S

.PHONY: all clean qemu debug
```

## Team

| Name              | Role                    | Primary Modules                    |
|-------------------|-------------------------|-------------------------------------|
| Bala Andegue      | Team Lead / Scheduler   | `proc/`, `scheduler.c`, `user/`     |
| Gabrielle Nana    | Memory Manager          | `mem/`, `virtual_memory.c`          |
| Israel Teme       | Syscall / Trap Engineer | `sys/`, `trap/`, `switch_context.S` |
| Tamwo Steveson    | FS / Driver Engineer    | `fs/`, `io/`, `virtio_disk.c`       |

## License

MIT License

Copyright (c) 2025 Bala Andegue, Gabrielle Nana, Israel Teme, Tamwo Steveson

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions...

## References

- [xv6 source code](https://pdos.csail.mit.edu/6.828/2020/xv6.html)
- [RISC-V Specification](https://riscv.org/technical/specifications/)
- [QEMU RISC-V virt machine docs](https://www.qemu.org/docs/master/system/target-riscv.html)
- [OS: Three Easy Pieces](https://pages.cs.wisc.edu/~remzi/OSTEP/)

---

**Status**: 🟡 In development – Week 1 (Boot + UART)  
**Last updated**: \today


