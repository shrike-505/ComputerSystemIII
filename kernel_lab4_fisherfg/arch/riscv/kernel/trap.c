#include <printk.h>
#include <stdint.h>
#include <syscalls.h>
#include <proc.h>

void clock_set_next_event(void);

extern struct task_struct *current;

void syscall_handler(struct pt_regs *regs){
  uint64_t syscall_num = regs->x[17];
  switch (syscall_num) {
    case __NR_write:
      if (regs->x[10] == 1) {
        char *buf = (char *)regs->x[11];
        for (uint64_t i = 0; i < regs->x[12]; i++) {
          printk("%c", buf[i]);
        }
        regs->x[10] = regs->x[12];
      }
      else {
        printk("[U] syscall write: unsupported fd = %ld", regs->x[10]);
        regs->x[10] = -1;
      }
      break;
    case __NR_getpid:
      regs->x[10] = current->pid;
      break;
    default:
      printk("[U] syscall: unsupported syscall number = %ld", syscall_num);
      regs->x[10] = -1;
      break;
  }
  regs->sepc += 4; // skip the ecall instruction
}

void trap_handler(struct pt_regs *regs, uint64_t scause, uint64_t stval) {
  if (scause == 0x8000000000000005){
    printk("[S] Supervisor timer interrupt at 0x%lx\n", stval);
    clock_set_next_event();
    do_timer();
  }
  else if (scause == 0x8){ //Environment call from U-mode
    syscall_handler(regs);
  }
  return;
}
