OUTPUT_ARCH( "riscv" )
ENTRY( _entry )

SECTIONS
{
  /* The kernel virtual address start */
  . = 0xffffffff80000000;

  _begin = .;

  .text : {
    KEEP(*(.text.boot))
    *(.text .text.*)
  }

  .rodata : ALIGN(4K) {
    *(.rodata .rodata.*)
  }

  .data : ALIGN(4K) {
    *(.data .data.*)
    *(.data.boot)
  }

  .sdata : ALIGN(4K) {
    /* Group small data together and anchor the global pointer here */
    PROVIDE(__global_pointer$ = . + 0x800);
    *(.sdata .sdata.*)
  }

  .bss : ALIGN(4K) {
    *(.sbss .sbss.*)
    *(.bss .bss.*)
  }

  _end = .;
}