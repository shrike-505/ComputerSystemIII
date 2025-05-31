#include <vm.h>
#include <mm.h>
#include <proc.h>
#include <printk.h>
#include <stdlib.h>
#include <string.h>
#include <private_kdefs.h>
#include <virtio.h>
#include <elf.h>
#include <fs.h>

//#define NO_PG_FAULT
//#define NO_FORK
//#define NO_ELF

#ifdef NO_FORK
#define INIT_TASKS NR_TASKS
#else
#define INIT_TASKS (1 + 1)
#endif

struct task_struct *task[NR_TASKS];        // 线程数组，所有的线程都保存在此
static struct task_struct *idle;           // idle 线程
struct task_struct *current;               // 当前运行线程

void __dummy(void);
void __switch_to(struct task_struct *prev, struct task_struct *next);

uint64_t avail_pid = INIT_TASKS;

extern uint64_t virtio_base;

struct vm_area_struct *find_vma(struct mm_struct *mm, void *va) {
    struct vm_area_struct *vma = mm->mmap;
    while (vma) {
        if ((uint64_t)vma->vm_start <= (uint64_t)va && (uint64_t)vma->vm_end > (uint64_t)va) {
            return vma;
        }
        vma = vma->vm_next;
    }
    return NULL;
}

void *do_mmap(struct mm_struct *mm, void *va, size_t len, unsigned flags) {
    if (va == NULL) {
        va = (void *)USER_START;
    }
    if ((uint64_t)va % PGSIZE != 0) {
        va = (void *)PGROUNDDOWN((uint64_t)va);
    }
    if ((uint64_t)len % PGSIZE != 0) {
        len = PGROUNDUP((uint64_t)len);
    }

    printk("[S] do_mmap: mm = %p, va = %p, len = %lx, flags = %x\n", mm, va, len, flags);

    struct vm_area_struct *new_vma = (struct vm_area_struct *)alloc_page();
    new_vma->vm_mm = mm;
    new_vma->vm_start = va;
    new_vma->vm_end = (void *)((uint64_t)va + len);
    new_vma->vm_flags = flags;
    new_vma->vm_next = mm->mmap;
    new_vma->vm_prev = NULL;
    if (mm->mmap) {
        mm->mmap->vm_prev = new_vma;
    }
    mm->mmap = new_vma;
    return va;
}

struct vm_area_struct* deepcopy_vma (struct vm_area_struct *vma ) {
    struct vm_area_struct *new_vma = (struct vm_area_struct *)alloc_page();
    new_vma->vm_mm = vma->vm_mm;
    new_vma->vm_start = vma->vm_start;
    new_vma->vm_end = vma->vm_end;
    new_vma->vm_flags = vma->vm_flags;
    new_vma->vm_prev = NULL;
    if (vma->vm_next) {
        new_vma->vm_next = deepcopy_vma(vma->vm_next);
        new_vma->vm_next->vm_prev = new_vma;
    }
    else {
        new_vma->vm_next = NULL;
    }
    return new_vma;
}

struct mm_struct* deepcopy_mm (struct mm_struct *mm) {
    struct mm_struct *new_mm = (struct mm_struct *)alloc_page();
    new_mm->mmap = deepcopy_vma(mm->mmap);
    struct vm_area_struct *vma = new_mm->mmap;
    while (vma) {
        vma->vm_mm = new_mm;
        vma = vma->vm_next;
    }
    return new_mm;
}

void load_elf(struct task_struct *task, char *start, char *end) {
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)start;
    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 || ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr->e_ident[EI_MAG2] != ELFMAG2 || ehdr->e_ident[EI_MAG3] != ELFMAG3) {
        printk("Invalid ELF file\n");
        return;
    }

    Elf64_Phdr *phdr = (Elf64_Phdr *)(start + ehdr->e_phoff);
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            size_t sz = PGROUNDUP(phdr[i].p_memsz);
            uint64_t va = PGROUNDDOWN(phdr[i].p_vaddr);
            uint64_t flags = 0;
            if (phdr[i].p_flags & PF_R) flags |= VM_READ;
            if (phdr[i].p_flags & PF_W) flags |= VM_WRITE;
            if (phdr[i].p_flags & PF_X) flags |= VM_EXEC;
            do_mmap(task->mm, (void *)va, sz, flags);

            char* va_cpy = (char *)alloc_pages( sz/PGSIZE + 1 );
            printk("Loading ELF segment: va = 0x%lx, sz = 0x%lx, flags = 0x%lx, offset = 0x%lx -> 0x%lx\n", va, sz, flags, phdr[i].p_offset, (uint64_t)va_cpy);
            extern char _suapp[], _euapp[];
            uint64_t va_src = USER_START + phdr[i].p_offset + (uint64_t)_suapp;
            //printk("Copying from source va = 0x%lx -> dest = 0x%lx\n", va_src, va_cpy);
            memcpy(va_cpy, (void *)va_src, sz);
            uint64_t pa = (uint64_t)va_cpy - PA2VA_OFFSET;
            uint64_t perm = PTE_V | PTE_U; // user space mapping
            if (flags & VM_READ) perm |= PTE_R;
            if (flags & VM_WRITE) perm |= PTE_W;
            if (flags & VM_EXEC) perm |= PTE_X;
            create_mapping(task->pgd, (void *)va, (void *)pa, sz, perm);
        }
    }

    task->thread.sepc = ehdr->e_entry; // set the entry point
    printk("Loaded ELF: entry point = 0x%lx\n", task->thread.sepc);
}

void task_init(void) {
    file_init();

    srand(2025);

    idle = (struct task_struct *)alloc_page();
    idle->state = TASK_RUNNING;
    idle->pid = 0;
    idle->priority = 0;
    idle->counter = 0;

    current = idle;
    task[0] = idle;

    for (int i = 1; i <= INIT_TASKS; i++){
        task[i] = (struct task_struct *)alloc_page();
        task[i]->state = TASK_RUNNING;
        task[i]->pid = i;
        task[i]->priority = PRIORITY_MIN + rand() % (PRIORITY_MAX - PRIORITY_MIN + 1);
        task[i]->counter = 0;

        task[i]->thread.ra = (uint64_t)__dummy;
        task[i]->thread.sp = (uint64_t)task[i] + PGSIZE;
        #ifdef NO_ELF
        task[i]->thread.sepc = (uint64_t)USER_START;
        #endif
        task[i]->thread.sstatus = (uint64_t)SSTATUS_SUM;
        task[i]->thread.sscratch = (uint64_t)USER_END;
        task[i]->thread.stval = 0;
        task[i]->thread.scause = 0;

        task[i]->pgd = (pagetable_t)alloc_page();
        memset(task[i]->pgd, 0, PGSIZE);

        extern uint64_t swapper_pg_dir[];
        //map kernel
        memcpy(task[i]->pgd, swapper_pg_dir, PGSIZE);

        extern char _suapp[], _euapp[];
        #ifdef NO_PG_FAULT
        char* task_uapp = (char*)alloc_pages( (uint64_t)(_euapp-_suapp) / PGSIZE + 1);
        memcpy(task_uapp, _suapp, _euapp - _suapp);
        #endif

        task[i]->mm = (struct mm_struct *)alloc_page();
        task[i]->mm->mmap = NULL;

        #ifdef NO_ELF
        //mmap uapp
        uint64_t va = USER_START;
        #ifdef NO_PG_FAULT
        uint64_t pa = (uint64_t)task_uapp - PA2VA_OFFSET;
        create_mapping(task[i]->pgd, (void *)va, (void *)pa, (uint64_t)(_euapp - _suapp), PTE_R | PTE_W | PTE_X | PTE_U | PTE_V);
        #else
        do_mmap(task[i]->mm, (void *)va, (uint64_t)(_euapp - _suapp), VM_READ | VM_WRITE | VM_EXEC);
        #endif
        #else
        //load elf
        load_elf(task[i], _suapp, _euapp);
        #endif

        //mmap user stack
        uint64_t u_stack_sz = 4 * PGSIZE;
        uint64_t va_sp = USER_END - u_stack_sz;
        #ifdef NO_PG_FAULT
        uint64_t pa_sp = (uint64_t)alloc_page() - PA2VA_OFFSET;
        create_mapping(task[i]->pgd, (void *)va_sp, (void *)pa_sp, PGSIZE, PTE_R | PTE_W | PTE_U | PTE_V);
        #else
        do_mmap(task[i]->mm, (void *)va_sp, u_stack_sz, VM_READ | VM_WRITE | VM_ANON);
        #endif

        //printk("INIT TASK [PID = %ld, PRIORITY = %ld, COUNTER = %ld, ra = %lx, sp = %lx]\n", task[i]->pid, task[i]->priority, task[i]->counter, task[i]->thread.ra, task[i]->thread.sp);
    }

    printk("...task_init done!\n");
}

void switch_to(struct task_struct *next) {
    if (current == next) return;
    struct task_struct *prev = current;
    current = next;
    printk("switch to [PID = %ld, PRIORITY = %ld, COUNTER = %ld]\n", next->pid, next->priority, next->counter);    
    __switch_to(prev, next);
}

void do_timer(void) {
  // 1. 如果当前线程时间片耗尽，则直接进行调度
  // 2. 否则将运行剩余时间减 1，若剩余时间仍然大于 0 则直接返回，否则进行调度
    //printk("timer interrupt at [PID = %ld, PRIORITY = %ld, COUNTER = %ld]\n", current->pid, current->priority, current->counter);
    if (current->counter == 0) {
        schedule();
    } else {
        current->counter--;
        if (current->counter == 0) {
            schedule();
        }
    }
}

void schedule(void) {
    struct task_struct *next = idle;
    for (uint64_t i = 1; i < avail_pid; i++){
        if (task[i]->state == TASK_RUNNING && task[i]->counter > next->counter){
            next = task[i];
        }
    }
    if (next == idle){
        for (uint64_t i = 1; i < avail_pid; i++){
            if (task[i]->state == TASK_RUNNING){
                task[i]->counter = task[i]->priority;
                #if USER_MAIN != SHELL
                printk("SET [PID = %ld, PRIORITY = %ld, COUNTER = %ld]\n", task[i]->pid, task[i]->priority, task[i]->counter);
                #endif
            }
            if (task[i]->state == TASK_RUNNING && task[i]->counter > next->counter){
            next = task[i];
            }
        }
    }
    switch_to(next);
}