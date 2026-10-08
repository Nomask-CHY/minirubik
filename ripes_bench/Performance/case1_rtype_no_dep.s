.globl _start
_start:
    li      t0, 500000       
    li      t1, 1
    li      t2, 2
    li      t3, 3
    li      t4, 4

loop:
    add     s0, t1, t2       # no data dependancy
    sub     s1, t3, t4
    xor     s2, t1, t3
    or      s3, t2, t4
    addi    t0, t0, -1
    bnez    t0, loop

    li      a7, 93
    li      a0, 0
    ecall