#include <vm.h>
#include <mm.h>
#include <string.h>
#include <printk.h>
#include <private_kdefs.h>
#include <stdint.h>
#include <sbi.h>

// 用于 setup_vm 进行 1 GiB 的映射
uint64_t early_pgtbl[PGSIZE / 8] __attribute__((__aligned__(PGSIZE)));
// kernel page table 根目录，在 setup_vm_final 进行映射
uint64_t swapper_pg_dir[PGSIZE / 8] __attribute__((__aligned__(PGSIZE)));

void setup_vm(void) {
    memset(early_pgtbl, 0, PGSIZE);

    // 1. 初始化阶段，页大小为 1 GiB，不使用多级页表                                                                                                                                        
    // 2. 将 va 的 64 bit 作如下划分：| 63...39 | 38...30 | 29...0 |
    //    - 63...39 bit 忽略
    //    - 38...30 bit 作为 early_pgtbl 的索引
    //    - 29...0 bit 作为页内偏移，注意到 30 = 9 + 9 + 12，即我们此处只使用根页表，根页表的每个 entry 对应 1 GiB 的页
    // 3. Page Table Entry 的权限为 X W R V

    uint64_t PA = PHY_START;
    int index = (PA >> 30) & 0x1ff;
    early_pgtbl[index] = ((PA >> 12) << 10) | 0xf | PTE_A | PTE_D;
    printk("..setup_vm, early_pgtbl[%d] = %lx\n", index, early_pgtbl[index]);
    index = (PA2VA(PA) >> 30) & 0x1ff;
    early_pgtbl[index] = ((PA >> 12) << 10) | 0xf | PTE_A | PTE_D;
    printk("..setup_vm, early_pgtbl[%d] = %lx\n", index, early_pgtbl[index]);
    printk("..setup_vm done!\n");
}


extern char _stext[], _etext[];
extern char _srodata[], _erodata[];
extern char _sdata[], _edata[];
void setup_vm_final(void) {
    memset(swapper_pg_dir, 0, PGSIZE);

    // No OpenSBI mapping required

    // 1. 调用 create_mapping 映射页表
    //    - kernel code: X R
    //    - kernel rodata: R
    //    - other memory: W R
    create_mapping(swapper_pg_dir, (uint64_t*)_stext, (uint64_t*)VA2PA(_stext), (uint64_t)(_etext-_stext), PTE_X | PTE_R | PTE_A | PTE_D);
    printk("..setup_vm_final, mapping kernel code\n");

    create_mapping(swapper_pg_dir, (uint64_t*)_srodata, (uint64_t*)VA2PA(_srodata), (uint64_t)(_erodata-_srodata), PTE_R | PTE_A | PTE_D);
    printk("..setup_vm_final, mapping kernel rodata\n");

    create_mapping(swapper_pg_dir, (uint64_t*)_sdata, (uint64_t*)VA2PA(_sdata), PHY_END + PA2VA_OFFSET - (uint64_t)_sdata, PTE_W | PTE_R | PTE_A | PTE_D);
    printk("..setup_vm_final, mapping kernel data\n");

    // 2. 设置 satp，将 swapper_pg_dir 作为内核页表
    // printk("..setup_vm_final, setting satp\n");
    uint64_t _satp = (VA2PA(swapper_pg_dir) >> 12) | (8L << 60);
    csr_write(satp, _satp);
    printk("..setup_vm_final satp: %lx\n", _satp);

    // flush TLB
    asm volatile("sfence.vma" ::: "memory");
    printk("..setup_vm_final done!\n");

    return;
}

void create_mapping(uint64_t pgtbl[static PGSIZE / 8], void *va, void *pa, uint64_t sz, uint64_t perm) {
    printk("..create_mapping va: %lx pa: %lx sz: %lx perm: %lx\n", (uint64_t)va, (uint64_t)pa, sz, perm);
    uint64_t VirtualAddr = (uint64_t)va;
    uint64_t PhysicalAddr = (uint64_t)pa;
    for (uint64_t va_end = (uint64_t)va + sz; VirtualAddr < va_end; VirtualAddr += PGSIZE, PhysicalAddr += PGSIZE) {
        uint64_t vpn[3] = {(VirtualAddr >> 30) & 0x1ff, (VirtualAddr >> 21) & 0x1ff, (VirtualAddr >> 12) & 0x1ff};
        uint64_t *pgtbl_entry = pgtbl;
        for (int level = 0; level < 2; ++level) {
            uint64_t pte = pgtbl_entry[vpn[level]];
            if ((pte & PTE_V) == 0) {
                uint64_t page = alloc_page();
                memset((void*)page, 0, PGSIZE);
                pte = (VA2PA(page) >> 12) << 10 | PTE_V;
                pgtbl_entry[vpn[level]] = pte;
            }
            pgtbl_entry = (uint64_t*)PA2VA((pte >> 10) << 12);
        }
        uint64_t pte = ((PhysicalAddr >> 12) << 10) | perm | PTE_V | PTE_A | PTE_D;
        pgtbl_entry[vpn[2]] = pte;
    }
}
