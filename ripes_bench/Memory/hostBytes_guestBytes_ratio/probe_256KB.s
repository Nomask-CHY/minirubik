.globl _start
_start:
    li t0, 0x10000000
    li t1, 262144
    li t2, 0x55
1:
    sb t2, 0(t0)
    addi t0, t0, 1
    addi t1, t1, -1
    bnez t1, 1b
    li a7, 93
    li a0, 0
    ecall
