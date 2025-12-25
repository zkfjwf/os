.globl kerneltrap
.globl kernelvec
.align 4

# kernelvec: 内核态中断入口
kernelvec:
        # 需要保存 32个寄存器 + sstatus + sepc
        # 34 * 8 = 272 字节
        addi sp, sp, -272

        # 保存 ra
        sd ra, 0(sp)

        # 保存“进入 kernelvec 前”的原始 sp
        addi t2, sp, 272
        sd t2, 8(sp)

        # 保存其余通用寄存器
        sd gp, 16(sp)
        sd tp, 24(sp)
        sd t0, 32(sp)
        sd t1, 40(sp)
        sd t2, 48(sp)
        sd s0, 56(sp)
        sd s1, 64(sp)
        sd a0, 72(sp)
        sd a1, 80(sp)
        sd a2, 88(sp)
        sd a3, 96(sp)
        sd a4, 104(sp)
        sd a5, 112(sp)
        sd a6, 120(sp)
        sd a7, 128(sp)
        sd s2, 136(sp)
        sd s3, 144(sp)
        sd s4, 152(sp)
        sd s5, 160(sp)
        sd s6, 168(sp)
        sd s7, 176(sp)
        sd s8, 184(sp)
        sd s9, 192(sp)
        sd s10, 200(sp)
        sd s11, 208(sp)
        sd t3, 216(sp)
        sd t4, 224(sp)
        sd t5, 232(sp)
        sd t6, 240(sp)

        # 保存 CSR
        csrr t0, sstatus
        csrr t1, sepc
        sd t0, 248(sp)
        sd t1, 256(sp)

        call kerneltrap

        # 恢复 CSR
        ld t0, 248(sp)
        csrw sstatus, t0

        # 恢复通用寄存器
        ld ra, 0(sp)
        ld t2, 8(sp)

        ld gp, 16(sp)
        ld tp, 24(sp)
        ld t0, 32(sp)
        ld t1, 40(sp)
        ld t2, 48(sp)
        ld s0, 56(sp)
        ld s1, 64(sp)
        ld a0, 72(sp)
        ld a1, 80(sp)
        ld a2, 88(sp)
        ld a3, 96(sp)
        ld a4, 104(sp)
        ld a5, 112(sp)
        ld a6, 120(sp)
        ld a7, 128(sp)
        ld s2, 136(sp)
        ld s3, 144(sp)
        ld s4, 152(sp)
        ld s5, 160(sp)
        ld s6, 168(sp)
        ld s7, 176(sp)
        ld s8, 184(sp)
        ld s9, 192(sp)
        ld s10, 200(sp)
        ld s11, 208(sp)
        ld t3, 216(sp)
        ld t4, 224(sp)
        ld t5, 232(sp)
        ld t6, 240(sp)

        # 回收栈并恢复原 sp
        mv sp, t2

        sret

