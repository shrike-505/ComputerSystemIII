#include <printk.h>
#include <readk.h>
#include <stdint.h>
#include <syscalls.h>
#include <ksyscalls.h>
#include <proc.h>
#include <vm.h>
#include <mm.h>
#include <string.h>
#include <private_kdefs.h>
#include <fs.h>

void clock_set_next_event(void);

extern struct task_struct *task[];
extern struct task_struct *current;

extern struct files_struct files;

extern uint64_t avail_file_desc;

struct dup dup_cxt[8];
int avail_dup = 0;

void syscall_handler(struct pt_regs *regs)
{
  uint64_t syscall_num = regs->x[17];
  int fd;
  struct file *file;
  switch (syscall_num)
  {
  case __NR_dup3:
  {
    int pid = regs->x[10];
    int old_fd = regs->x[11];
    int new_fd = regs->x[12];
    dup_cxt[avail_dup].pid = pid;
    dup_cxt[avail_dup].old_fd = old_fd;
    dup_cxt[avail_dup].new_fd = new_fd;
    avail_dup++;
  }
  case __NR_open:
  {
    const char *path = (const char *)regs->x[10];
    int flags = regs->x[11];
    fd = file_open(&files.fd_array[avail_file_desc], path, flags);
    if (fd >= 0)
    {
      regs->x[10] = avail_file_desc; // return the fd
      avail_file_desc++;
    }
    else
    {
      printk("[U] syscall open: failed to open file %s\n", path);
      regs->x[10] = -1; // error
    }
    break;
  }
  case __NR_close:
  {
    fd = regs->x[10];
    if (fd >= 0 && fd < MAX_FILE_NUMBER && files.fd_array[fd].opened)
    {
      file = &files.fd_array[fd];
      file_close(file);
      regs->x[10] = 0; // success
    }
    else
    {
      printk("[U] syscall close: unsupported fd = %ld\n", regs->x[10]);
      regs->x[10] = -1; // error
    }
    break;
  }
  case __NR_lseek:
  {
    fd = regs->x[10];
    for (int i = 0; i < avail_dup; i++)
    {
      if (dup_cxt[i].pid == current->pid && dup_cxt[i].old_fd == fd)
      {
        fd = dup_cxt[i].new_fd;
        break;
      }
    }
    file = &files.fd_array[fd];
    if (fd >= 0 && fd < MAX_FILE_NUMBER && file->opened)
    {
      int64_t offset = regs->x[11];
      uint64_t whence = regs->x[12];
      if (file->lseek)
      {
        regs->x[10] = file->lseek(file, offset, whence);
      }
      else
      {
        printk("[U] syscall lseek: lseek not implemented for fd = %ld\n", fd);
        regs->x[10] = -1; // error
      }
    }
    else
    {
      printk("[U] syscall lseek: unsupported fd = %ld\n", regs->x[10]);
      regs->x[10] = -1; // error
    }
    break;
  }
  case __NR_read:
  {
    fd = regs->x[10];
    file = &files.fd_array[fd];
    if ((fd == 0 || fd >= 3) && fd < MAX_FILE_NUMBER && file->opened && (file->perms & F_READ))
    {
      char *buf = (char *)regs->x[11];
      regs->x[10] = file->read(file, buf, regs->x[12]);
    }
    else
    {
      printk("[U] syscall read: unsupported fd = %ld\n", regs->x[10]);
      regs->x[10] = -1;
    }
    break;
  }
  case __NR_write:
  {
    fd = regs->x[10];
    file = &files.fd_array[fd];
    if (fd >= 1 && fd < MAX_FILE_NUMBER && file->opened && (file->perms & F_WRITE))
    {
      char *buf = (char *)regs->x[11];
      regs->x[10] = file->write(file, buf, regs->x[12]);
    }
    else
    {
      printk("[U] syscall write: unsupported fd = %ld\n", regs->x[10]);
      regs->x[10] = -1;
    }
    break;
  }
  case __NR_getpid:
  {
    regs->x[10] = current->pid;
    break;
  }
  case __NR_exit:
  {
    int error_code = regs->x[10];
    printk("[U] syscall exit: pid = %ld, error_code = %d\n", current->pid, error_code);
    current->state = TASK_DEAD; // mark the current task as dead
    // for (int i = 0; i < avail_dup; i++)
    // {
    //   if (dup_cxt[i].pid == current->pid)
    //   {
    //     for (int j = i; j < avail_dup - 1; j++)
    //     {
    //       dup_cxt[j] = dup_cxt[j + 1];
    //     }
    //     avail_dup--;
    //     i--; // adjust index after removal
    //   }
    // }
    schedule(); // switch to another task
    break;
  }
  case __NR_waitpid:
  {
    int pid = regs->x[10];
    int *status = (int *)regs->x[11];
    int options = regs->x[12];
    if (pid < 0 || pid >= NR_TASKS)
    {
      printk("[U] syscall waitpid: unsupported pid = %d\n", pid);
      regs->x[10] = -1; // error
    }
    // struct wait_t wait;
    // wait.parent = current->pid;
    // wait.child = pid;
    // wait_table[avail_wait] = wait;
    // avail_wait = (avail_wait + 1) % NR_TASKS;
    task[pid]->priority = current->priority + 1;
    task[pid]->counter = task[pid]->priority;
    while (1)
    {
      // wait for the child process to change state
      if (task[pid]->state == TASK_ZOMBIE || task[pid]->state == TASK_DEAD)
      {
        regs->x[10] = pid; // return the pid
        task[pid]->state = TASK_DEAD; // mark as dead
        free_pid(pid); // free the pid
        current->state = TASK_RUNNING; // set current task to running state
        break;
      }
      // yield to other processes
      current->state = TASK_INTERRUPTIBLE; // set current task to interruptible state
      schedule();
    }
    break;
  }
  case __NR_clone:
  {
    printk("[U] syscall clone @ pid = %ld, sepc = %lx\n", current->pid, regs->sepc);
    regs->x[10] = sys_clone(regs);
    break;
  }
  case __NR_execve:
  {
    printk("[U] syscall execve @ pid = %ld, sepc = %lx\n", current->pid, regs->sepc);
    const char *filename = (const char *)regs->x[10];
    char *const *argv = (char *const *)regs->x[11];
    char *const *envp = (char *const *)regs->x[12];
    regs->x[10] = sys_execve(filename, argv, envp);
    if (regs->x[10] != -1)
    {
      regs->sepc = current->thread.sepc;
    }
    break;
  }
  default:
  {
    printk("[U] syscall: unsupported syscall number = %ld\n", syscall_num);
    regs->x[10] = -1;
    break;
  }
  }
  regs->sepc += 4; // skip the ecall instruction
}

static void do_page_fault(struct pt_regs *regs, uint64_t scause, uint64_t stval)
{
  printk("[S] do_page_fault: scause = 0x%lx, stval = 0x%lx, sepc = 0x%lx\n", scause, stval, regs->sepc);
  uint64_t bad_va = stval;
  struct vm_area_struct *vma = find_vma(current->mm, (void *)bad_va);

  if (vma == NULL)
  {
    printk("[S] do_page_fault: no vma found for va = 0x%lx\n", bad_va);
    regs->x[10] = -1; // error
    return;
  }
  unsigned flags = vma->vm_flags;
  switch (scause)
  {
  case 0xc: // Instruction page fault
    if (!(flags & VM_EXEC))
    {
      printk("[S] do_page_fault: instruction page fault at va = 0x%lx, but no execute permission\n", bad_va);
      regs->x[10] = -1; // error
      return;
    }
    break;
  case 0xd: // Load page fault
    if (!(flags & VM_READ))
    {
      printk("[S] do_page_fault: load page fault at va = 0x%lx, but no read permission\n", bad_va);
      regs->x[10] = -1; // error
      return;
    }
    break;
  case 0xf: // Store page fault
    if (!(flags & VM_WRITE))
    {
      printk("[S] do_page_fault: store page fault at va = 0x%lx, but no write permission\n", bad_va);
      regs->x[10] = -1; // error
      return;
    }
    break;
  default:
    printk("[S] do_page_fault: unsupported scause = 0x%lx\n", scause);
    regs->x[10] = -1; // error
    return;
  }

  unsigned is_anon = flags & VM_ANON;
  uint64_t perm = ((flags & ~VM_ANON) << 1) | PTE_V | PTE_U;

  // copy_on_write shared page
  if (scause == 0xf)
  {
    uint64_t *pgtbl = current->pgd;
    uint64_t vpn2 = (bad_va >> 30) & 0x1ff;
    uint64_t pte = pgtbl[vpn2];

    if (pte & PTE_V)
    {
      pgtbl = (uint64_t *)(((pte >> 10) << 12) + PA2VA_OFFSET);
      uint64_t vpn1 = (bad_va >> 21) & 0x1ff;
      pte = pgtbl[vpn1];

      if (pte & PTE_V)
      {
        pgtbl = (uint64_t *)(((pte >> 10) << 12) + PA2VA_OFFSET);
        uint64_t vpn0 = (bad_va >> 12) & 0x1ff;
        pte = pgtbl[vpn0];

        if ((pte & PTE_V) && !(pte & PTE_W) && (pte & PTE_S))
        {
          uint64_t pa = (pte >> 10) << 12;
          uint64_t pa_v = pa + PA2VA_OFFSET;

          char *va_cpy = (char *)alloc_page();
          memcpy(va_cpy, (void *)pa_v, PGSIZE);
          uint64_t new_pa = (uint64_t)va_cpy - PA2VA_OFFSET;
          printk("[S] copy on write: pid = %ld, copy %lx from %lx to %lx\n", current->pid, (uint64_t)bad_va, pa, new_pa);
          bad_va = PGROUNDDOWN(bad_va);
          create_mapping(current->pgd, (void *)bad_va, (void *)new_pa, PGSIZE, perm);

          deref_page((void *)pa_v);
          return;
        }
      }
    }
  }

  if (is_anon)
  {
    uint64_t pa = (uint64_t)alloc_page() - PA2VA_OFFSET;
    bad_va = PGROUNDDOWN(bad_va);
    create_mapping(current->pgd, (void *)bad_va, (void *)pa, PGSIZE, perm);
  }
  else
  {
    extern char _suapp[];
    char *va_cpy = (char *)alloc_page();
    memcpy(va_cpy, (void *)(PGROUNDDOWN((uint64_t)bad_va + (uint64_t)_suapp)), PGSIZE);
    uint64_t pa = (uint64_t)va_cpy - PA2VA_OFFSET;
    bad_va = PGROUNDDOWN(bad_va);
    create_mapping(current->pgd, (void *)bad_va, (void *)pa, PGSIZE, perm);
  }
}

void trap_handler(struct pt_regs *regs, uint64_t scause, uint64_t stval)
{
  // printk("[S] trap_handler: scause = 0x%lx, stval = 0x%lx\n", scause, stval);
  if (scause == 0x8000000000000005)
  {
    // printk("[S] Supervisor timer interrupt at 0x%lx\n", stval);
    clock_set_next_event();
    do_timer();
  }
  else if (scause == 0x8)
  { // Environment call from U-mode
    syscall_handler(regs);
  }
  else if (scause == 0xc || scause == 0xd || scause == 0xf)
  { // Page Fault
    do_page_fault(regs, scause, stval);
  }
  else
  {
    printk("[S] trap_handler: unsupported scause = 0x%lx, stval = 0x%lx\n", scause, stval);
  }
  return;
}
