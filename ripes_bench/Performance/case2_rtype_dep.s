.globl _start
_start:
    li      t0, 500000
    li      t1, 1
    li      t2, 2

loop:
    add     t1, t1, t2       
    add     t1, t1, t2       # Forwarding 
    add     t1, t1, t2
    add     t1, t1, t2
    addi    t0, t0, -1
    bnez    t0, loop

    li      a7, 93
    li      a0, 0
    ecall