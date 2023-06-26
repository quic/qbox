PHDRS
{
    segment_code            PT_LOAD FLAGS(5);
}

SECTIONS
{
    . = 0x0;
    .text :
    {
        KEEP(*(.text._start));
    } :segment_code
}
