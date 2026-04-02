MEMORY
{
    ram      (rx)  : ORIGIN = 0x0000, LENGTH = 0x1000
    rom      (rx)  : ORIGIN = 0x1000, LENGTH = 0x1000
}

SECTIONS
{
    .data :
    {
        *(.data*)
    } > ram

    .binmem :
    {
        *(.binmem*)
    } > rom
}
