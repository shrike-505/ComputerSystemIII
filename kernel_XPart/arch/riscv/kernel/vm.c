#include <mm.h>
#include <vm.h>
#include <printk.h>
#include <string.h>

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
  printk("setup_vm: early_pgtbl = %p\n", early_pgtbl);

  uint64_t pa = PHY_START;
  uint64_t va = VM_START;
  uint64_t p_index = (pa >> 30) & 0x1ff; //等值映射
  uint64_t v_index = (va >> 30) & 0x1ff;
  uint64_t perm = PTE_R | PTE_W | PTE_X;

  early_pgtbl[p_index] = (((pa >> 30) & 0x3ffffff) << 28) | perm | PTE_V; //等值映射
  early_pgtbl[v_index] = (((pa >> 30) & 0x3ffffff) << 28) | perm | PTE_V;

  printk("setup_vm: early_pgtbl[%lu] = 0x%lx\n", p_index, early_pgtbl[p_index]);
  printk("setup_vm: early_pgtbl[%lu] = 0x%lx\n", v_index, early_pgtbl[v_index]);


}

void setup_vm_final(void) {
  memset(swapper_pg_dir, 0, PGSIZE);

  // No OpenSBI mapping required

  // 1. 调用 create_mapping 映射页表
  //    - kernel code: X R
  //    - kernel rodata: R
  //    - other memory: W R
  // 2. 设置 satp，将 swapper_pg_dir 作为内核页表

  uint64_t pa = PHY_START + OPENSBI_SIZE;
  uint64_t va = VM_START + OPENSBI_SIZE;

  // uint64_t text_size = 0x2000;
  // uint64_t rodata_size = 0x1000;

  extern char _stext[];
  extern char _srodata[];
  extern char _sdata[];
  printk("setup_vm_final: _stext = %p, _srodata = %p, _sdata = %p\n", _stext, _srodata, _sdata);

  uint64_t text_size = (uint64_t)_srodata - (uint64_t)_stext; //4kB align
  uint64_t rodata_size = (uint64_t)_sdata - (uint64_t)_srodata;

  uint64_t other_size = PHY_SIZE - text_size - rodata_size;
  uint64_t text_perm = PTE_R | PTE_X;
  uint64_t rodata_perm = PTE_R;
  uint64_t other_perm = PTE_R | PTE_W;

  create_mapping(swapper_pg_dir, (void *)va, (void *)pa, text_size, text_perm | PTE_V);
  create_mapping(swapper_pg_dir, (void *)(va + text_size), (void *)(pa + text_size), rodata_size, rodata_perm | PTE_V);
  create_mapping(swapper_pg_dir, (void *)(va + text_size + rodata_size), (void *)(pa + text_size + rodata_size), other_size, other_perm | PTE_V);

  uint64_t satp = (0x8000000000000000 | ((uint64_t)swapper_pg_dir - PA2VA_OFFSET) >> 12);
  printk("setup_vm_final: satp = 0x%lx\n", satp);
  asm volatile("csrw satp, %0" : : "r"(satp) : "memory");

  // flush TLB
  asm volatile("sfence.vma" ::: "memory");

  return;
}

void create_mapping(uint64_t pgtbl[static PGSIZE / 8], void *va, void *pa, uint64_t sz, uint64_t perm) {
  // TODO：根据 RISC-V Sv39 的要求，创建多级页表映射关系
  //
  // 物理内存需要分页
  // 创建多级页表的时候使用 alloc_page 来获取新的一页作为页表
  // 注意通过 V bit 来判断表项是否存在
  //
  // 重要：阅读手册，注意 A / D 位的设置

  printk("create_mapping pgtbl=%lx: [0x%lx, 0x%lx) -> [0x%lx, 0x%lx), perm = 0x%lx\n",
    (uint64_t)pgtbl - PA2VA_OFFSET, (uint64_t)va, (uint64_t)va+sz, (uint64_t)pa, (uint64_t)pa+sz, perm);

  uint64_t va_start = (uint64_t)va;
  uint64_t pa_start = (uint64_t)pa;
  uint64_t va_end = va_start + sz;
  
  uint64_t pai = pa_start;
  uint64_t vai = va_start;

  uint64_t *tbl, vpn, pte;
  while(vai < va_end)
  {
    //创建第一级
    tbl = pgtbl; 
    vpn = (vai >> 30) & 0x1ff;
    pte = tbl[vpn];

    if(!(pte & PTE_V))
    {
      uint64_t new_page_p = (uint64_t)(alloc_page()) - PA2VA_OFFSET;
      pte = (new_page_p >> 12) << 10 | PTE_V;
      tbl[vpn] = pte;
    }

    //创建第二级
    tbl = (uint64_t *)( ((pte >> 10) << 12) + PA2VA_OFFSET);
    vpn = (vai >> 21) & 0x1ff;
    pte = tbl[vpn];

    if(!(pte & PTE_V))
    {
      uint64_t new_page_p = (uint64_t)(alloc_page()) - PA2VA_OFFSET;
      pte = (new_page_p >> 12) << 10 | PTE_V;
      tbl[vpn] = pte;
    }

    //创建第三级
    tbl = (uint64_t *)( ((pte >> 10) << 12) + PA2VA_OFFSET);
    vpn = (vai >> 12) & 0x1ff;
    pte = (pai >> 12) << 10 | perm | PTE_V;
    tbl[vpn] = pte;

    //更新va和pa
    vai += PGSIZE;
    pai += PGSIZE;

  }
}
