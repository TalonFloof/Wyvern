ENTRY(_start)

SECTIONS {
    . = 0x08000000;

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
}