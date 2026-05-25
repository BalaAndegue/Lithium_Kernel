# TODO – Lithium Kernel

## Bloc 1 : Boot + Console (Semaine 1)
- [x] kernel/entry.S
- [x] kernel/kernel.ld
- [x] kernel/io/uart.c
- [x] kernel/main.c

## Bloc 2 : Mémoire (Semaine 2)
- [x] kernel/mem/physmem.c
- [x] kernel/mem/paging.c
- [ ] kernel/mem/operations.c
- [ ] kernel/mem/virtual_memory.c

## Bloc 3 : Processus (Semaine 3-4)
- [x] kernel/proc/process.c
- [x] kernel/proc/control.c
- [x] kernel/proc/scheduler.c
- [x] kernel/proc/switch_context.S

## Bloc 4 : Syscalls + Traps (Semaine 5)
- [x] kernel/trap/trap.c
- [x] kernel/trap/plic.c
- [x] kernel/sys/syscall.c
- [x] include/riscv/riscv_defs.h
- [x] include/riscv/intrinsics.h

## Bloc 5 : FS + Drivers (Semaine 6-7)
- [x] kernel/mem/spinlock.c
- [x] kernel/fs/sleeplock.c
- [x] kernel/fs/buffer.c
- [x] kernel/fs/block.c
- [x] kernel/fs/log.c
- [x] kernel/fs/inode.c
- [x] kernel/fs/file.c
- [x] kernel/fs/pipe.c
- [x] kernel/fs/file_system.c
- [x] kernel/fs/virtio_disk.c
- [ ] kernel/mem/operations.c
- [ ] kernel/mem/virtual_memory.c

## Bloc 6 : User Space (Semaine 8)
- [x] user/init.c
- [x] user/ulibc/ulibc.c
- [x] include/user/syscall.h
- [x] include/user/ulibc.h
