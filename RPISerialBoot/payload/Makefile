# payload/Makefile
CROSS   = aarch64-elf
CC      = $(CROSS)-gcc
AS      = $(CROSS)-gcc
LD      = $(CROSS)-ld
OBJCOPY = $(CROSS)-objcopy

CFLAGS  = -ffreestanding -nostdlib -nostartfiles \
          -mgeneral-regs-only \
          -Wall -Wextra -O2

SRCS    = start.S main.c
OBJS    = start.o main.o

all: payload.img

start.o: start.S
	$(AS) $(CFLAGS) -c $< -o $@

main.o: main.c
	$(CC) $(CFLAGS) -c $< -o $@

payload.elf: $(OBJS)
	$(LD) -T link.ld -o $@ $(OBJS)

payload.img: payload.elf
	$(OBJCOPY) -O binary $< $@
	@echo "payload.img ready -- $$(wc -c < payload.img) bytes"

clean:
	rm -f $(OBJS) payload.elf payload.img

.PHONY: all clean