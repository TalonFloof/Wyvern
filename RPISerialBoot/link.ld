ENTRY(_start)

SECTIONS {
    /* Bootloader lives at 0x100000, well above the 0x80000
     * payload load address so we don't clobber ourselves
     */
    . = 0x80000;

    .text : {
        KEEP(*(.text.boot))     /* start.S entry point must be first */
        *(.text)
        *(.text.*)
    }

    .rodata : {
        *(.rodata)
        *(.rodata.*)
    }

    .data : {
        *(.data)
        *(.data.*)
    }

    /* 8 byte aligned BSS — our clear loop increments by 8 */
    . = ALIGN(8);
    .bss : {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(8);
        __bss_end = .;
    }

    /* Stack grows down from 0x100000 toward lower addresses
     * payload sits at 0x80000 giving us 512KB of stack space
     */
    __stack_top = 0x100000;
}