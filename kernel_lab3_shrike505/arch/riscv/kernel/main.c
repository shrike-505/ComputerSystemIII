#include <printk.h>
#include <sbi.h>
#include <private_kdefs.h>

extern char _stext[];
extern char _srodata[];

_Noreturn void start_kernel(){
    // csr_write(sscratch, 0x7e8);
    // printk("%lu", csr_read(sscratch));
    printk("2025 ZJU Computer System III\n");
    // printk("_stext = %x\n", *_stext);
    // printk("_srodata = %x\n", *_srodata);
    // *_stext = 0;
    // *_srodata = 0;
    // printk("_stext = %x\n", *_stext);
    // printk("_srodata = %x\n", *_srodata);

    while(1);
}