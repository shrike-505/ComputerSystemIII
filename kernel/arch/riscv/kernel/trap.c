#include <stdint.h>
#include <printk.h>
#include <proc.h>

void clock_set_next_event(void);

void trap_handler(uint64_t scause, uint64_t sepc) {
    // 根据 scause 判断 trap 类型
    // 如果是 Supervisor Timer Interrupt：
    // - 打印输出相关信息
    // - 调用 clock_set_next_event 设置下一次时钟中断
    // 其他类型的 trap 可以直接忽略，推荐打印出来供以后调试
    int interrupt = scause >> 63;
    int exceptionCode = scause & 0x7FFFFFFFFFFFFFFF; // 与上01111....1，去除最高位
    // printk("scause = %lx, sepc = %llx", scause, sepc);
    switch (interrupt) {
        case 1:
            switch (exceptionCode) {
                case 1:
                    printk("[S] Supervisor software interrupt\n");
                    // Maybe a func
                    break;
                case 5:
                    // printk("[S] Supervisor timer interrupt\n");
                    clock_set_next_event();
                    do_timer();
                    break;
                case 9:
                    printk("[S] Supervisor external interrupt\n");
                    // Maybe a func
                    break;
                case 13:
                    printk("Counter-overflow interrupt\n");
                    // Maybe a func
                    break;
                default:
                    // other interrupts, Reserved or for platform use
                    break;
            }
            break;
        case 0:
            switch (exceptionCode)
            {
            case 0:
                printk("Instruction address misaligned\n");
                // Maybe a func
                break;
            case 1:
                printk("Instruction access fault\n");
                // Maybe a func
                break;
            case 7:
                printk("Store/AMO access fault\n");
                break;
            case 12:
                printk("Instruction page fault\n");
                // Maybe a func
                break;
            default:
                break;
            }
            
        default:
            break;
    }
}
