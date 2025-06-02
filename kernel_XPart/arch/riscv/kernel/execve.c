#include <execve.h>

extern uint64_t virtio_base;
extern struct files_struct files;
extern uint64_t avail_file_desc;

int do_execve(const char *filename, char *const argv[], char *const envp[]) {
    int fd = file_open(&files.fd_array[avail_file_desc], filename, F_READ | F_EXEC);
    if (fd < 0) {
        printk("[U] execve: failed to open file %s\n", filename);
        return -1;
    }

    struct file *file = &files.fd_array[fd];
    if (file->fs_type != FS_TYPE_FAT32) {
        printk("[U] execve: unsupported file system type for %s\n", filename);
        file_close(file);
        return -1;
    }

    struct fat32_file *fat32_file = &file->fat32_file;
    uint64_t file_size = fat32_file->size;
    if (file_size == 0) {
        printk("[U] execve: file %s is empty\n", filename);
        file_close(file);
        return -1;
    }
    
    uint64_t elf_start = (uint64_t)alloc_pages((file_size + PGSIZE - 1) / PGSIZE + 1);
    uint64_t elf_end = elf_start + PGROUNDUP(file_size);
    fat32_read_file(virtio_base, fat32_file, (void *)elf_start, file_size);
    Elf64_Ehdr* ehdr = (Elf64_Ehdr *)elf_start;
    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 || ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr->e_ident[EI_MAG2] != ELFMAG2 || ehdr->e_ident[EI_MAG3] != ELFMAG3) {
        printk("[U] execve: %s is not a valid ELF file\n", filename);
        file_close(file);
        return -1;
    }
    printk("[U] execve: elf entry point = 0x%lx\n", ehdr->e_entry);

    printk("[U] execve: %s is a valid ELF file, size: %lu bytes\n", filename, file_size);    
    extern struct task_struct *current;

    //current->thread.ra = (uint64_t)__dummy;
    current->thread.sp = (uint64_t)current + PGSIZE;
    current->thread.sstatus = (uint64_t)SSTATUS_SUM;
    current->thread.sscratch = (uint64_t)USER_END;
    current->thread.stval = 0;
    current->thread.scause = 0;

    current->pgd = (pagetable_t)alloc_page();
    memset(current->pgd, 0, PGSIZE);
    extern uint64_t swapper_pg_dir[];
    memcpy(current->pgd, swapper_pg_dir, PGSIZE);
    printk("[U] execve: current->pgd = 0x%lx\n", (uint64_t)current->pgd);

    current->mm = (struct mm_struct *)alloc_page();
    current->mm->mmap = NULL;

    load_elf(current, (char *)elf_start, (char *)elf_end);

    uint64_t u_stack_sz = 4 * PGSIZE;
    uint64_t va_sp = USER_END - u_stack_sz;
    do_mmap(current->mm, (void *)va_sp, u_stack_sz, VM_READ | VM_WRITE | VM_ANON); 

    uint64_t u_heap_sz = USER_HEAP_SIZE;
    uint64_t va_heap = USER_HEAP;
    do_mmap(current->mm, (void *)va_heap, u_heap_sz, VM_READ | VM_WRITE | VM_ANON);   

    uint64_t pgd_pa = (uint64_t)current->pgd - PA2VA_OFFSET;
    uint64_t satp = (pgd_pa >> 12) | (0x8000000000000000); 
    asm volatile("csrw satp, %0" : : "r"(satp));
    asm volatile("sfence.vma zero, zero" ::: "memory");
    
    current->thread.sepc = ehdr->e_entry; // Set the entry point

    // asm volatile("csrw sepc, %0" : : "r"(ehdr->e_entry)); // Set entry point
    // asm volatile("csrw sstatus, %0" : : "r"(SSTATUS_SUM)); // Set status to user mode with interrupts enabled
    // asm volatile("csrw sscratch, %0" : : "r"(USER_END)); // Set scratch register for user space
    // asm volatile("csrw stval, zero"); // Clear stval
    // asm volatile("csrw scause, zero"); // Clear scause

    // printk("[U] execve: switching to user mode with satp = 0x%lx\n", satp);
    // asm volatile("sret"); // Return to user mode

    file_close(file);
    return 0; // Success
}
