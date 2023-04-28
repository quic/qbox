ENTRY(_start)

MEMORY {
  rom (rx)  : ORIGIN = 0x00000000, LENGTH = 0x10000
  ram (rwx) : ORIGIN = 0x00010000, LENGTH = 0x10000
}

SECTIONS
{
  .reset : {
    *setup.s.o(.text)
  } > rom

  .text : {
    *(.text)
    *(.note.gnu.build-id)
    *(.ARM.attributes)
    *(.rodata .rodata.*)
  } > rom

  .data : {
    *(.data)
  } > ram

  .bss : {
    . = ALIGN (8);
    __bss_start = . ;
    *(.shbss)
    *(.bss .bss.*)
    *(COMMON)
    . = ALIGN (8);
    __bss_end = .;
  } > ram

  .stack : {
    . += 0x1000;
    . = ALIGN (8);
    _initial_sp = .;
    . += 0x1000;
    . = ALIGN (8);
    _initial_irq_sp = .;
  } > ram
}

