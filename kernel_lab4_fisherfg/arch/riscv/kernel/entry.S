#include <private_kdefs.h>

    .section .text.entry
    .align 4

    .globl _traps
_traps:
    # 0. 若用户态中断，切换栈
    csrr t0, sscratch
    beq t0, zero, _1  # 内核态sscratch==0 
    csrw sscratch, sp
    mv sp, t0

    # 1. 将寄存器和 sepc 保存到栈上
    # x0 - x31 和 sepc 从低地址到高地址依次保存
_1:
    sd sp, -256(sp)
    sd ra, -248(sp)
    sd gp, -240(sp)
    sd tp, -232(sp)
    sd t0, -224(sp)
    sd t1, -216(sp)
    sd t2, -208(sp)
    sd s0, -200(sp)
    sd s1, -192(sp)
    sd a0, -184(sp)
    sd a1, -176(sp)
    sd a2, -168(sp)
    sd a3, -160(sp)
    sd a4, -152(sp)
    sd a5, -144(sp)
    sd a6, -136(sp)
    sd a7, -128(sp)
    sd s2, -120(sp)
    sd s3, -112(sp)
    sd s4, -104(sp)
    sd s5, -96(sp)
    sd s6, -88(sp)
    sd s7, -80(sp)
    sd s8, -72(sp)
    sd s9, -64(sp)
    sd s10, -56(sp)
    sd s11, -48(sp)
    sd t3, -40(sp)
    sd t4, -32(sp)
    sd t5, -24(sp)
    sd t6, -16(sp)
    csrr t0, sepc
    sd t0, -8(sp)
    addi sp, sp, -264

    # 2. 调用 trap_handler
    # void trap_handler(struct pt_regs *regs, uint64_t scause, uint64_t stval)
    mv a0, sp
    csrr a1, scause
    csrr a2, sepc
    call trap_handler

    # 3. 恢复寄存器和 sepc
    ld t0, 256(sp)
    csrw sepc, t0
    ld t6, 248(sp)
    ld t5, 240(sp)
    ld t4, 232(sp)
    ld t3, 224(sp)
    ld s11, 216(sp)
    ld s10, 208(sp)
    ld s9, 200(sp)
    ld s8, 192(sp)
    ld s7, 184(sp)
    ld s6, 176(sp)
    ld s5, 168(sp)
    ld s4, 160(sp)
    ld s3, 152(sp)
    ld s2, 144(sp)
    ld a7, 136(sp)
    ld a6, 128(sp)
    ld a5, 120(sp)
    ld a4, 112(sp)
    ld a3, 104(sp)
    ld a2, 96(sp)
    ld a1, 88(sp)
    ld a0, 80(sp)
    ld s1, 72(sp)
    ld s0, 64(sp)
    ld t2, 56(sp)
    ld t1, 48(sp)
    ld t0, 40(sp)
    ld tp, 32(sp)
    ld gp, 24(sp)
    ld ra, 16(sp)
    ld sp, 8(sp)

    # 4. 返回
    csrr t0, sscratch
    beq t0, zero, _2  
    csrw sscratch, sp
    mv sp, t0

_2:
    sret

    .globl __dummy
__dummy:
    csrr t0, sscratch
    csrw sscratch, sp
    mv sp, t0
    csrwi sepc, 0
    sret


    .globl __switch_to
__switch_to:
    # 1. 将当前线程的上下文保存到当前线程的 thread_struct 中
    sd ra, 32(a0)
    sd sp, 40(a0)
    sd s0, 48(a0)
    sd s1, 56(a0)
    sd s2, 64(a0)
    sd s3, 72(a0)
    sd s4, 80(a0)
    sd s5, 88(a0)
    sd s6, 96(a0)
    sd s7, 104(a0)
    sd s8, 112(a0)
    sd s9, 120(a0)
    sd s10, 128(a0)
    sd s11, 136(a0)
    csrr t1, sepc
    sd t1, 144(a0)
    csrr t1, sstatus
    sd t1, 152(a0)
    csrr t1, sscratch
    sd t1, 160(a0)
    csrr t1, stval 
    sd t1, 168(a0)
    csrr t1, scause 
    sd t1, 176(a0)
    # 保存当前进程的页表
    csrr t1, satp
    li t0, 0xfffff
    and t1, t1, t0
    slli t1, t1, 12
    li t2, PA2VA_OFFSET
    add t1, t1, t2
    sd t1, 184(a0)

    # 2. 从下一个线程的 thread_struct 中恢复上下文
    ld ra, 32(a1)
    ld sp, 40(a1)
    ld s0, 48(a1)
    ld s1, 56(a1)
    ld s2, 64(a1)
    ld s3, 72(a1)
    ld s4, 80(a1)
    ld s5, 88(a1)
    ld s6, 96(a1)
    ld s7, 104(a1)
    ld s8, 112(a1)
    ld s9, 120(a1)
    ld s10, 128(a1)
    ld s11, 136(a1)
    ld t1, 144(a1)
    csrw sepc, t1
    ld t1, 152(a1)
    csrw sstatus, t1
    ld t1, 160(a1)
    csrw sscratch, t1
    ld t1, 168(a1)
    csrw stval, t1
    ld t1, 176(a1)
    csrw scause, t1
    # 恢复下一个进程的页表，并载入 satp
    ld t1, 184(a1)  
    li t2, PA2VA_OFFSET
    sub t1, t1, t2
    li t0, 8
    srli t1, t1, 12
    slli t0, t0, 60
    or t1, t1, t0
    csrw satp, t1 
    sfence.vma zero, zero  

    ret