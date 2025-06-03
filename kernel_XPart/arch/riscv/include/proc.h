#ifndef __PROC_H__
#define __PROC_H__

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>

#define TASK_RUNNING 0 
#define TASK_INTERRUPTIBLE 1
#define TASK_UNINTERRUPTIBLE 2
#define TASK_ZOMBIE 3
#define TASK_STOPPED 4
#define TASK_DEAD 5
#define TASK_TRACED 6
#define TASK_CONTINUED 7

//#define NO_PG_FAULT

// 可自行修改的宏定义
#define NR_TASKS (1 + 8) // idle 线程 + 用户线程
#define PRIORITY_MIN 1
#define PRIORITY_MAX 10

typedef uint64_t *pagetable_t;

// 中断处理所需寄存器堆
struct pt_regs {
  uint64_t x[32];
  uint64_t sepc;
};

// 线程状态结构
struct thread_struct {
  uint64_t ra;
  uint64_t sp;
  uint64_t s[12];

  uint64_t sepc;
  uint64_t sstatus;
  uint64_t sscratch;
  uint64_t stval;
  uint64_t scause;
};

#define VM_READ 0x01
#define VM_WRITE 0x02
#define VM_EXEC 0x04
#define VM_ANON 0x08

struct vm_area_struct {
  /**
   * @brief The mm_struct we belong to.
   */
  struct mm_struct *vm_mm;

  /**
   * @brief Our start address within vm_mm.
   */
  void *vm_start;

  /**
   * @brief The past-the-end address within vm_mm.
   */
  void *vm_end;

  /**
   * @brief Flags as listed above.
   */
  unsigned vm_flags;

  /**
   * @brief linked list of VM areas per task, sorted by address.
   */
  struct vm_area_struct *vm_prev;
  struct vm_area_struct *vm_next;
};

struct mm_struct {
  /**
   * @brief list of VMAs.
   */
  struct vm_area_struct *mmap;
};


/**
 * @brief 遍历 mm，寻找 va 所在的 vm_area_struct
 *
 * @param mm 进程的 mm_struct
 * @param va 虚拟地址
 *
 * @return va 所在的 vm_area_struct 结构体指针，若未找到则返回 NULL
 */
struct vm_area_struct *find_vma(struct mm_struct *mm, void *va);

/**
 * @brief 向 mm 中添加一个 vm_area_struct
 *
 * @param mm 进程的 mm_struct
 * @param vm 要添加的 vm_area_struct 的起始地址
 * @param len vm_area_struct 记录的长度
 * @param flags vm_area_struct 的权限位
 *
 * @return 该映射的起始地址
 */
void *do_mmap(struct mm_struct *mm, void *va, size_t len, unsigned flags);

struct vm_area_struct* deepcopy_vma (struct vm_area_struct *vma );
struct mm_struct* deepcopy_mm (struct mm_struct *mm);

// 进程数据结构
struct task_struct {
  uint64_t pid;      // 进程 ID
  uint64_t state;    // 状态
  uint64_t priority; // 优先级
  uint64_t counter;  // 剩余时间

  struct thread_struct thread; // 线程结构

  pagetable_t pgd; // 页表

  struct mm_struct *mm;
};

/**
 * @brief 进程初始化函数
 */
void task_init(void);

/**
 * @brief 时钟中断处理函数
 */
void do_timer(void);

/**
 * @brief 进程调度函数
 */
void schedule(void);

/**
 * @brief 切换到下一个进程
 *
 * @param next 要切换到的进程
 */
void switch_to(struct task_struct *next);

/**
 * @brief 空闲线程函数
 */
void dummy_task(void);

uint64_t get_next_avail_pid(void);
void free_pid(uint64_t pid);

struct dup{
  int pid;
  int old_fd;
  int new_fd;
};

#endif
