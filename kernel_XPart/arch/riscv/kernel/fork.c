#include <fork.h>

long do_fork(struct pt_regs *regs){
  extern struct task_struct *current;

  struct task_struct *child = (struct task_struct *)alloc_page();

  child->state = TASK_RUNNING;
  child->pid = get_next_avail_pid();
  if (child->pid < 0) {
    printk("[S] fork: no available pid\n");
    return -1; // no available pid
  }

  printk("[S] fork: %ld -> %ld\n", current->pid, child->pid);

  child->priority = PRIORITY_MIN + rand() % (PRIORITY_MAX - PRIORITY_MIN + 1);
  child->counter = 0;

  extern void ret_from_fork(void);
  child->thread.ra = (uint64_t)&ret_from_fork;
  child->thread.sp = (uint64_t)child - (uint64_t)current + (uint64_t)regs;
  asm volatile("csrr %0, sscratch" : "=r"(child->thread.sscratch));
  child->thread.sepc = regs->sepc + 4;

  child->thread.sstatus = current->thread.sstatus;
  child->thread.stval = current->thread.stval;
  child->thread.scause = current->thread.scause;

  struct pt_regs *child_regs = (struct pt_regs *)child->thread.sp;
  for (int i = 0; i < 32; i++) {
    child_regs->x[i] = regs->x[i];
  }
  child_regs->x[10] = 0; // return 0 to child
  child_regs->x[1] = child->thread.sp;
  child_regs->sepc = child->thread.sepc;

  child->mm = deepcopy_mm(current->mm);

  child->pgd = (pagetable_t)alloc_page();
  memset(child->pgd, 0, PGSIZE);
  extern uint64_t swapper_pg_dir[];
  memcpy(child->pgd, swapper_pg_dir, PGSIZE);

  for (uint64_t * pgt2 = current->pgd; pgt2 < current->pgd + PGSIZE / 8; pgt2++) {
    if (*pgt2 & PTE_V) {
      uint64_t *pgt10 = (uint64_t *)( (((uint64_t)*pgt2 >> 10) << 12) + PA2VA_OFFSET);
      uint64_t va2 = (((uint64_t)pgt2 - (uint64_t)current->pgd) / sizeof(uint64_t)) << 30; //VPN2->VA[38:30]
      for (uint64_t * pgt1 = pgt10; pgt1 < pgt10 + PGSIZE / 8; pgt1++) {
        if (*pgt1 & PTE_V) {
          uint64_t *pgt00 = (uint64_t *)( (((uint64_t)*pgt1 >> 10) << 12) + PA2VA_OFFSET);
          uint64_t va1 = (((uint64_t)pgt1 - (uint64_t)pgt10) / sizeof(uint64_t)) << 21; //VPN1->VA[29:21]
          for (uint64_t * pgt0 = pgt00; pgt0 < pgt00 + PGSIZE / 8; pgt0++) {
            if (*pgt0 & PTE_V) {
              uint64_t va0 = (((uint64_t)pgt0 - (uint64_t)pgt00) / sizeof(uint64_t)) << 12; //VPN0->VA[20:12]
              uint64_t va = va2 | va1 | va0;
              if (va >> 38 == 0) { //user pgtbl
                uint64_t pa = (*pgt0 >> 10) << 12; //PA[51:12]
                printk("[S] ref page: va = 0x%lx", va);
                uint64_t pa_v = pa + PA2VA_OFFSET;
                printk(" -> 0x%lx\n", pa_v);
                ref_page((void *)pa_v);
                *pgt0 = (*pgt0 | PTE_S) & ~PTE_W; //clear W bit   
                create_mapping(child->pgd, (void *)va, (void *)pa, PGSIZE, *pgt0 & 0x1ff); //deepcopy pgtbl            
              }
            }
          }
        }
      }
    }
  }
  asm volatile("sfence.vma" ::: "memory");

//   memcpy(child->pgd, current->pgd, PGSIZE);
//   printk("[S] copy pgtbl: %lx -> %lx\n", (uint64_t)current->pgd - PA2VA_OFFSET, (uint64_t)child->pgd - PA2VA_OFFSET);
 
  extern struct task_struct *task[];
  task[child->pid] = child;

  return child->pid;    
}