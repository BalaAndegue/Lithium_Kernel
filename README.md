
# Lithium Kernel

A minimal UNIX-like kernel for RISC-V architecture.  
Inspired by xv6, rewritten and modularized for learning and experimentation.

## Table of Contents

- [Architecture](#architecture)
- [Project Structure](#project-structure)
- [Quick Start](#quick-start)
- [Build Commands](#build-commands)
- [Module Breakdown](#module-breakdown)
- [Team](#team)
- [License](#license)



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
├── kernel/
│   ├── entry.S                 # Boot entry point (assembly)
│   ├── kernel.ld               # Linker script
│   ├── main.c                  # kernel_main() – initialization
│   ├── setup.c                 # Low-level CPU setup
│   │
│   ├── fs/                     # File system
│   │   ├── fs.c                # Block cache, superblock
│   │   ├── inode.c             # Inode management
│   │   └── file.c              # File descriptors
│   │
│   ├── io/                     # Drivers
│   │   ├── uart.c              # Serial console (NS16550a)
│   │   ├── virtio.c            # VirtIO block device
│   │   └── console.c           # printf() wrapper
│   │
│   ├── mem/                    # Memory management
│   │   ├── physmem.c           # Physical page allocator (bitmap)
│   │   └── paging.c            # SV39 page tables
│   │
│   ├── proc/                   # Process management
│   │   ├── proc.c              # fork(), exec(), exit(), wait()
│   │   ├── scheduler.c         # Round-robin scheduler
│   │   └── switch.S            # Context switch (assembly)
│   │
│   ├── sys/                    # System calls
│   │   └── syscall.c           # Syscall dispatcher
│   │
│   └── trap/                   # Traps & interrupts
│       ├── trap.c              # Unified trap handler
│       └── plic.c              # Timer interrupts
│
├── include/                    # Shared headers
│   ├── types.h                 # uint32, uint64, etc.
│   ├── defs.h                  # Common prototypes
│   ├── riscv.h                 # CSR macros
│   └── spinlock.h              # Synchronization primitives
│
├── user/                       # User space
│   ├── init.c                  # First user program
│   ├── ulib.c                  # Syscall wrappers
│   └── printf.c                # User-space printf()
│
├── tools/
│   └── mkfs.c                  # Disk image builder
│
└── make_fs/
    └── fs.img                  # Generated disk image

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

### 2. Clone / Create Project

```bash
cd ~/lithium-kernel
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

---

## Build Commands

| Command       | Description                                      |
|---------------|--------------------------------------------------|
| `make`        | Build kernel image (`kernel/kernel.elf`)         |
| `make clean`  | Remove all object files and kernel image         |
| `make qemu`   | Launch QEMU with the kernel                      |
| `make debug`  | Launch QEMU in debug mode (listening on port 1234)|
| `make fs`     | Build disk image (`make_fs/fs.img`)              |
| `make all`    | Build kernel + disk image                        |

---

## Module Breakdown

| Module     | Files                                      | Description                                                                 |
|------------|--------------------------------------------|-----------------------------------------------------------------------------|
| **Boot**   | `entry.S`, `kernel.ld`, `setup.c`          | CPU initialization, stack setup, jump to `kernel_main()`                    |
| **Memory** | `physmem.c`, `paging.c`                    | Physical page allocator (first-fit bitmap), SV39 page tables                |
| **Proc**   | `proc.c`, `scheduler.c`, `switch.S`        | Process lifecycle (`fork`, `exec`, `exit`, `wait`), round-robin scheduler   |
| **Trap**   | `trap.c`, `plic.c`                         | Exception handling, interrupt routing, timer interrupts                     |
| **Syscall**| `syscall.c`                                | User→kernel interface (`write`, `read`, `fork`, `exec`, `exit`, `wait`)      |
| **Driver** | `uart.c`, `virtio.c`, `console.c`          | Serial output, block device I/O                                             |
| **FS**     | `fs.c`, `inode.c`, `file.c`                | Simple UNIX file system (inodes, directories, block cache)                  |
| **User**   | `init.c`, `ulib.c`                         | First user process, system call wrappers                                    |

---

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

---

## Coding Conventions

| Rule                          | Rationale                                     |
|-------------------------------|-----------------------------------------------|
| 4-space indentation, no tabs  | Consistency across editors                    |
| `snake_case` for functions    | UNIX kernel style                             |
| `UPPER_CASE` for macros       | Standard C practice                           |
| `/* comment */` for multi-line| C89 compatibility (some toolchains)           |
| `// comment` for single-line  | Permitted in C17                              |
| No dynamic allocation after boot | Deterministic memory usage                 |
| Every `alloc` has matching `free` | Prevent memory leaks                      |
| Validate all user pointers    | Security (kernel must not crash on bad input) |

---

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

---

## Performance Notes

| Component           | Current | Target | Notes                                    |
|---------------------|---------|--------|------------------------------------------|
| Physical allocator  | O(n)    | O(1)   | First-fit bitmap is fine for <128MB      |
| Page table walk     | O(3)    | O(3)   | SV39 fixed 3-level walk                  |
| Scheduler           | O(n)    | O(1)   | Round-robin with runqueue improves later |
| Block cache         | LRU     | LRU    | 16-entry cache is sufficient             |

---

## Future Improvements (Post-2 Months)

- [ ] Copy-on-Write (COW) for `fork()`
- [ ] Demand paging (lazy allocation)
- [ ] Symmetric Multi-Processing (SMP)
- [ ] Network driver (VirtIO-Net)
- [ ] ELF loader with shared libraries
- [ ] Pipe implementation for IPC
- [ ] Signals and alarm()

---

## Team

| Name              | Role                    | Primary Modules                    |
|-------------------|-------------------------|-------------------------------------|
| Bala Andegue      | Team Lead / Scheduler   | `proc/`, `scheduler.c`, `user/`     |
| Gabrielle Nana    | Memory Manager          | `mem/`, `paging.c`, `physmem.c`     |
| Israel Teme       | Syscall / Trap Engineer | `sys/`, `trap/`, `switch.S`         |
| Tamwo Steveson    | FS / Driver Engineer    | `fs/`, `io/`, `virtio.c`, `uart.c`  |

---

## License

MIT License

Copyright (c) 2025 Bala Andegue, Gabrielle Nana, Israel Teme, Tamwo Steveson

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions...

---

## References

- [xv6 source code](https://pdos.csail.mit.edu/6.828/2020/xv6.html)
- [RISC-V Specification](https://riscv.org/technical/specifications/)
- [QEMU RISC-V virt machine docs](https://www.qemu.org/docs/master/system/target-riscv.html)
- [OS: Three Easy Pieces](https://pages.cs.wisc.edu/~remzi/OSTEP/)

---

**Status**: 🟡 In development – Week 1 (Boot + UART)  
**Last updated**: \today
