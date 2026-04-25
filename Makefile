
CROSS_COMPILE = riscv64-unknown-elf-
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
QEMU = qemu-system-riscv64

CFLAGS = -Wall -Werror -O2 -fno-omit-frame-pointer -ggdb -gdwarf-2
CFLAGS += -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib
CFLAGS += -mno-relax -I include

LDFLAGS = -T kernel/kernel.ld -z max-page-size=4096

# ============================================================================
# Bloc 1: Boot + UART
# ============================================================================
BLOCK1_OBJS = \
	kernel/entry.o \
	kernel/main.o

# ============================================================================
# Bloc 2: Memory Management
# ============================================================================
BLOCK2_OBJS = \
	kernel/io/uart.o \
	kernel/io/console.o \
	kernel/mem/physmem.o \
	kernel/mem/paging.o

# ============================================================================
# Bloc 3: Processus (à décommenter plus tard)
# ============================================================================
# BLOCK3_OBJS = \
# 	kernel/proc/process.o \
# 	kernel/proc/scheduler.o \
# 	kernel/proc/switch_context.o

# ============================================================================
# Bloc 4: System calls (à décommenter plus tard)
# ============================================================================
# BLOCK4_OBJS = \
# 	kernel/sys/syscall.o

# ============================================================================
# Assemblage final
# ============================================================================
KERNEL_OBJS = $(BLOCK1_OBJS) $(BLOCK2_OBJS)
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
