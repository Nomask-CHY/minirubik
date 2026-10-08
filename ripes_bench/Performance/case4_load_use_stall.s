.globl _start
_start:
    li      t0, 500000
    la      a1, dummy_mem
    li      t1, 10

loop:
    lw      t3, 0(a1)        
    add     t4, t3, t1       # load-use
    lw      t5, 0(a1)        
    add     t6, t5, t1       # 1-cycle bubbl
    addi    t0, t0, -1
    bnez    t0, loop

    li      a7, 93
    li      a0, 0
    ecall

.data
dummy_mem: .word 42