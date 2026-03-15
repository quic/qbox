ENTRY(_start)

SECTIONS
{
    . = 0x0;

    .text : {
        *(.text)
        *(.text.*)
    }

    . = ALIGN(8);

    .rodata : {
        *(.rodata)
        *(.rodata.*)
    }

    . = ALIGN(8);

    .data : {
        *(.data)
        *(.data.*)
    }

    . = ALIGN(8);

    .bss : {
        *(.bss)
        *(.bss.*)
    }
}
