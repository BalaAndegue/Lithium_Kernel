CROSS_COMPILE = riscv64-unknown-elf-
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
QEMU = qemu-system-riscv64

# Options du compilateur C
CFLAGS = -Wall -Werror -O0 -fno-omit-frame-pointer -ggdb -gdwarf-2
CFLAGS += -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib
CFLAGS += -mno-relax -I include

# Options de l'éditeur de liens
LDFLAGS = -T kernel/kernel.ld -z max-page-size=4096

# ===========================================================================
# Bloc 1: Boot + UART (démarrage et communication)
# ===========================================================================
BLOCK1_OBJS = \
	kernel/entry.o \
	kernel/main.o

# ===========================================================================
# Bloc 2: Gestion de la mémoire
# ===========================================================================
BLOCK2_OBJS = \
	kernel/io/uart.o \
	kernel/io/console.o \
	kernel/string.o \
	kernel/mem/physmem.o \
	kernel/mem/paging.o

# ===========================================================================
# Bloc 3: Gestion des processus
# ===========================================================================
BLOCK3_OBJS = \
	kernel/proc/process.o \
	kernel/proc/control.o \
	kernel/proc/scheduler.o \
	kernel/proc/switch_context.o

# ===========================================================================
# Bloc 4: Pièges + Appels système
# ===========================================================================
BLOCK4_OBJS = \
	kernel/trap/trap.o \
	kernel/trap/plic.o \
	kernel/sys/syscall.o

# ===========================================================================
# Lier tous les fichiers pour créer le kernel
# ===========================================================================
KERNEL_OBJS = $(BLOCK1_OBJS) $(BLOCK2_OBJS) $(BLOCK3_OBJS) $(BLOCK4_OBJS)
KERNEL = kernel/kernel.elf

# Règle par défaut: construire le kernel
all: $(KERNEL)

# Lier les objets en un seul exécutable
$(KERNEL): $(KERNEL_OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

# Compiler les fichiers C en objets
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Compiler les fichiers assembleur en objets
%.o: %.S
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(KERNEL_OBJS) $(KERNEL)
	find . -name "*.d" -delete

# Exécuter le kernel dans QEMU
qemu: $(KERNEL)
	$(QEMU) -machine virt -bios none -kernel $(KERNEL) -nographic

# Exécuter en mode debug (attendre gdb)
debug: $(KERNEL)
	$(QEMU) -machine virt -bios none -kernel $(KERNEL) -nographic -s -S

.PHONY: all clean qemu debug