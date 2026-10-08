.data
dummy_mem: .word 42

.text
.globl _start
_start:
    li      t0, 500000
    la      a1, dummy_mem
    li      t1, 10
    li      t2, 20

loop:
    lw      t3, 0(a1)
    add     s0, t1, t2
    add     s1, t3, t1
    sub     s2, t2, t1
    addi    t0, t0, -1
    bnez    t0, loop

    li      a7, 93
    li      a0, 0
    ecall