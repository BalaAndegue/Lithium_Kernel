
# Lithium Kernel

A minimal UNIX-like kernel for RISC-V architecture.  
Inspired by xv6, rewritten and modularized for learning and experimentation.

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
├── README.md
├── TODO.md
├── .gitignore
├── .gdbinit
│
├── include/
│   ├── kernel/
│   │   ├── fs/                 # FS headers (block, buffer, file, inode, log, pipe, virtio)
│   │   ├── io/                 # I/O headers (console, debug, uart, printk, trace)
│   │   ├── mem/                # Memory headers (kmalloc, layout, operations, spinlock)
│   │   ├── proc/               # Process headers (control, elf, process, scheduler)
│   │   ├── sys/                # Syscall headers (number, syscall)
│   │   ├── trap/               # Trap headers (plic, trap)
│   │   ├── param.h
│   │   └── types.h
│   ├── make_fs/                # Disk image builder headers
│   ├── riscv/                  # RISC-V specific headers
│   └── user/                   # User space headers
│
├── kernel/
│   ├── entry.S                 # Boot entry point (assembly)
│   ├── kernel.ld               # Linker script
│   ├── main.c                  # kernel_main() – initialization
│   ├── setup.c                 # Low-level CPU setup
│   ├── fs/                     # File system implementation
│   ├── io/                     # Drivers (UART, console, debug)
│   ├── mem/                    # Memory management
│   ├── proc/                   # Process management + scheduler
│   └── sys/                    # System calls
│
├── make_fs/                    # Disk image build directory
├── tools/                      # Host utilities
└── user/                       # User programs
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

Expected output:
```
Lithium Kernel starting...
uart initialized.
mem_init: 128 MB available
proc_init: 64 process slots ready
timer_init: CLINT configured
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
| `make fs`     | Build disk image (`make_fs/fs.img`)              |
| `make all`    | Build kernel + disk image                        |

## Module Breakdown

| Module     | Location               | Description                                                                 |
|------------|------------------------|-----------------------------------------------------------------------------|
| **Boot**   | `entry.S`, `setup.c`   | CPU initialization, stack setup, jump to `kernel_main()`                    |
| **Memory** | `mem/`                 | Physical page allocator (first-fit bitmap), SV39 page tables                |
| **Proc**   | `proc/`                | Process lifecycle (`fork`, `exec`, `exit`, `wait`), round-robin scheduler   |
| **Trap**   | `trap/`                | Exception handling, interrupt routing, timer interrupts                     |
| **Syscall**| `sys/`                 | User→kernel interface (`write`, `read`, `fork`, `exec`, `exit`, `wait`)      |
| **Driver** | `io/`                  | Serial output (UART), console I/O                                           |
| **FS**     | `fs/`                  | Simple UNIX file system (inodes, directories, block cache, VirtIO disk)     |
| **User**   | `user/`                | First user process (`init`), system call wrappers                           |

## Key Data Structures

```c
// Process
struct proc {
    uint64 sz;                // Process memory size
    pagetable_t pagetable;    // Page table
    uint64 kstack;            // Kernel stack
    enum proc_state state;    // UNUSED, USED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE
    int pid;                  // Process ID
    struct proc *parent;      // Parent process
    uint64 trapframe;         // Saved registers
    uint64 context;           // Switch context
};

// Inode
struct inode {
    uint dev;                 // Device number
    uint inum;                // Inode number
    int ref;                  // Reference count
    short type;               // File, directory, device
    short nlink;              // Number of links
    uint size;                // File size in bytes
    uint addrs[NDIRECT+1];    // Direct + indirect block pointers
};

// Page Table Entry (SV39)
typedef uint64 pte_t;
#define PTE_V (1 << 0)        // Valid
#define PTE_R (1 << 1)        // Read
#define PTE_W (1 << 2)        // Write
#define PTE_X (1 << 3)        // Execute
#define PTE_U (1 << 4)        // User accessible
```

## Coding Conventions

| Rule                          | Rationale                                     |
|-------------------------------|-----------------------------------------------|
| 4-space indentation, no tabs  | Consistency across editors                    |
| `snake_case` for functions    | UNIX kernel style                             |
| `UPPER_CASE` for macros       | Standard C practice                           |
| `/* comment */` for multi-line| C89 compatibility                             |
| `// comment` for single-line  | Permitted in C17                              |
| No dynamic allocation after boot | Deterministic memory usage                 |
| Every `alloc` has matching `free` | Prevent memory leaks                      |
| Validate all user pointers    | Security (kernel must not crash on bad input) |

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
```

## Fichiers supplémentaires à créer

Pour que le `README.md` soit complet, assurez-vous d'avoir aussi ces fichiers :

### `.gitignore`
```
*.o
*.elf
*.img
*.bin
*.log
build/
```

### `.gdbinit`
```
set confirm off
target remote :1234
break main.c:42
continue
