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
