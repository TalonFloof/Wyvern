CROSS   = aarch64-elf
CC      = $(CROSS)-gcc
AS      = $(CROSS)-gcc
LD      = $(CROSS)-ld
OBJCOPY = $(CROSS)-objcopy

# -ffreestanding: no standard library assumed
# -nostdlib: don't link standard startup files
# -nostartfiles: don't use standard system startup
# -mgeneral-regs-only: no SIMD/FP registers, safer for baremetal
CFLAGS  = -ffreestanding -nostdlib -nostartfiles \
          -mgeneral-regs-only \
          -Wall -Wextra \
          -O2

ASFLAGS = -ffreestanding -nostdlib -nostartfiles

SRCS    = start.S uart.c main.c
OBJS    = $(SRCS:.S=.o)
OBJS   := $(OBJS:.c=.o)

all: kernel8.img

%.o: %.S
	$(AS) $(ASFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel8.elf: $(OBJS)
	$(LD) -nostdlib -T link.ld -o $@ $(OBJS)

kernel8.img: kernel8.elf
	$(OBJCOPY) -O binary $< $@
	@echo "kernel8.img ready -- $$(wc -c < kernel8.img) bytes"

clean:
	rm -f $(OBJS) kernel8.elf kernel8.img

.PHONY: all clean