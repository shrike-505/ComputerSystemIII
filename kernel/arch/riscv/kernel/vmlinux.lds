OUTPUT_ARCH("riscv")

/* 程序入口 */
ENTRY(_start)

PHY_START    = 0x80000000;
PHY_SIZE     = (128 * 1024 * 1024);
PHY_END      = (PHY_START + PHY_SIZE);
PGSIZE       = 0x1000;
OPENSBI_SIZE = (0x200000);
VM_START     = (0xffffffe000000000);
VM_END       = (0xffffffff00000000);
VM_SIZE      = (VM_END - VM_START);
PA2VA_OFFSET = (VM_START - PHY_START);

MEMORY {
    ram  (wxa!ri): ORIGIN = PHY_START + OPENSBI_SIZE, LENGTH = PHY_SIZE - OPENSBI_SIZE
    ramv (wxa!ri): ORIGIN =  VM_START + OPENSBI_SIZE, LENGTH =  VM_SIZE - OPENSBI_SIZE
}

/* kernel 代码起始位置 */
BASE_ADDR = VM_START + OPENSBI_SIZE;

SECTIONS
{
    /* . 代表当前地址 */
    . = BASE_ADDR;

    /* 记录 kernel 代码的起始地址 */
    _skernel = .;

    /* ALIGN(0x1000) 表示 4 KiB 对齐 */
    /* _stext, _etext 分别记录了 text 段的起始与结束地址 */
    .text : ALIGN(0x1000) {
        _stext = .;

        *(.text.init)
        *(.text.entry)
        *(.text .text.*)

        _etext = .;
    } >ramv AT>ram

    .rodata : ALIGN(0x1000) {
        _srodata = .;

        *(.rodata .rodata.*)

        _erodata = .;
    } >ramv AT>ram

    .data : ALIGN(0x1000) {
        _sdata = .;

        *(.data .data.*)
        *(.got .got.*)

        _edata = .;
    } >ramv AT>ram

    .bss : ALIGN(0x1000) {
        *(.bss.stack)
        . = ALIGN(0x1000);
        _sbss = .;

        *(.sbss .sbss.*)
        *(.bss .bss.*)

        _ebss = .;
    } >ramv AT>ram

    /* 记录 kernel 代码的结束地址 */
    . = ALIGN(0x1000);
    _ekernel = .;
}
