#include <vm.h>
#include <mm.h>
#include <proc.h>
#include <printk.h>
#include <stdlib.h>
#include <string.h>
#include <private_kdefs.h>

static struct task_struct *task[NR_TASKS]; // 线程数组，所有的线程都保存在此
static struct task_struct *idle;           // idle 线程
struct task_struct *current;               // 当前运行线程

void __dummy(void);
void __switch_to(struct task_struct *prev, struct task_struct *next);

// 在这里添加或实现这些函数：
// - void dummy_task(void);
// - void task_init(void);
// - void do_timer(void);
// - void schedule(void);
// - void switch_to(struct task_struct* next);

// void dummy_task(void) {
//   unsigned local = 0;
//   unsigned prev_cnt = 0;
//   while (1) {
//     if (current->counter != prev_cnt) {
//       if (current->counter == 1) {
//         // 若 priority 为 1，则线程可见的 counter 永远为 1（为什么？）
//         // 通过设置 counter 为 0，避免信息无法打印的问题
//         current->counter = 0;
//       }
//       prev_cnt = current->counter;
//       printk("[PID = %lu @ 0x%lx] Running. local = %u\n", current->pid, (uint64_t)current, ++local);
//     }
//   }
// }

void task_init(void) {
    srand(2025);

    idle = (struct task_struct *)alloc_page();
    idle->state = TASK_RUNNING;
    idle->pid = 0;
    idle->priority = 0;
    idle->counter = 0;

    current = idle;
    task[0] = idle;

    for (int i = 1; i < NR_TASKS; i++){
        task[i] = (struct task_struct *)alloc_page();
        task[i]->state = TASK_RUNNING;
        task[i]->pid = i;
        task[i]->priority = PRIORITY_MIN + rand() % (PRIORITY_MAX - PRIORITY_MIN + 1);
        task[i]->counter = 0;

        task[i]->thread.ra = (uint64_t)__dummy;
        task[i]->thread.sp = (uint64_t)task[i] + PGSIZE;
        task[i]->thread.sepc = (uint64_t)USER_START;
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
        char* task_uapp = (char*)alloc_pages( (uint64_t)(_euapp-_suapp) / PGSIZE + 1);
        memcpy(task_uapp, _suapp, _euapp - _suapp);

        //map uapp
        uint64_t va = USER_START;
        uint64_t pa = (uint64_t)task_uapp - PA2VA_OFFSET;
        create_mapping(task[i]->pgd, (void *)va, (void *)pa, (uint64_t)(_euapp - _suapp), PTE_R | PTE_W | PTE_X | PTE_U | PTE_V);

        //map user stack
        va = USER_END - PGSIZE;
        pa = (uint64_t)alloc_page() - PA2VA_OFFSET;
        create_mapping(task[i]->pgd, (void *)va, (void *)pa, PGSIZE, PTE_R | PTE_W | PTE_U | PTE_V);

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
    for (int i = 1; i < NR_TASKS; i++){
        if (task[i]->state == TASK_RUNNING && task[i]->counter > next->counter){
            next = task[i];
        }
    }
    if (next == idle){
        for (int i = 1; i < NR_TASKS; i++){
            if (task[i]->state == TASK_RUNNING){
                task[i]->counter = task[i]->priority;
                printk("SET [PID = %ld, PRIORITY = %ld, COUNTER = %ld]\n", task[i]->pid, task[i]->priority, task[i]->counter);
            }
            if (task[i]->state == TASK_RUNNING && task[i]->counter > next->counter){
            next = task[i];
            }
        }
    }
    switch_to(next);
}