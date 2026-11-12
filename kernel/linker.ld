OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

SECTIONS
{
    . = 0x00100000;

    .text ALIGN(0x1000) :
    {
        KEEP(*(.text.entry))
        *(.text .text.*)
    }

    .rodata ALIGN(0x1000) :
    {
        *(.rodata .rodata.*)
    }

    .data ALIGN(0x1000) :
    {
        *(.data .data.*)
    }

    .bss ALIGN(0x1000) (NOLOAD) :
    {
        *(COMMON)
        *(.bss .bss.*)
    }

    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame*)
        *(.note*)
    }
}
