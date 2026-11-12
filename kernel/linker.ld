OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

PHDRS
{
    text PT_LOAD FLAGS(5);   /* R-X */
    rodata PT_LOAD FLAGS(4); /* R-- */
    data PT_LOAD FLAGS(6);   /* RW- */
}

SECTIONS
{
    . = 0xffffffff80000000;

    .text ALIGN(0x1000) :
    {
        KEEP(*(.text.entry))
        *(.text .text.*)
    } :text

    .rodata ALIGN(0x1000) :
    {
        *(.rodata .rodata.*)
    } :rodata

    .limine_requests ALIGN(0x1000) :
    {
        KEEP(*(.limine_requests))
    } :data

    .data ALIGN(0x1000) :
    {
        *(.data .data.*)
    } :data

    .bss ALIGN(0x1000) (NOLOAD) :
    {
        *(COMMON)
        *(.bss .bss.*)
    } :data

    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame*)
        *(.note*)
    }
}
