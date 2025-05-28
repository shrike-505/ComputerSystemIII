OUTPUT_ARCH("riscv")

ENTRY(_start)

SECTIONS
{
    . = 0;

    .text : {
        _ustext = .;
        *(.text.init)
        *(.text .text.*)
        _uetext = .;
    }

    .data : {
        _usdata = .;
        *(.data .data.*)
        _uedata = .;
    }

    .rodata : {
        _usrodata = .;
        *(.rodata .rodata*)
        _uerodata = .;
    }

    .bss : {
        _usbss = .;
        *(.sbss .sbss.*)
        *(.bss .bss.*)
        _uebss = .;
    }
}
