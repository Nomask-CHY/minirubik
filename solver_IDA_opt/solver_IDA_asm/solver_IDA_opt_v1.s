    .data
# Test case: p[0..6] then o[0..6], 1-based, exactly 14 characters
#   "12345671111111" 0 / "25713642221111" 8 / "21345671111111" 11
test_str:
    .string "21345671111111"
    .byte 0                     # pad to 16 bytes

    .text
main:
    addi sp, sp, -16

    # Parse test_str into state.p / state.o
    la   t1, test_str
    li   t2, 0
m_parse:
    li   t3, 14
    bge  t2, t3, m_parse_end
    add  t4, t1, t2
    lbu  t4, 0(t4)
    addi t4, t4, -49
    mv   a0, t2
    li   a1, 7
    jal  ra, __umodsi3
    mv   t6, a0
    li   t5, 7
    blt  t2, t5, m_parse_store
    addi t6, t6, 7
m_parse_store:
    add  t6, sp, t6
    sb   t4, 0(t6)
    addi t2, t2, 1
    j    m_parse
m_parse_end:

    # moves = solve_ida(&state); result_depth = moves
    mv   a0, sp
    jal  ra, solve_ida
    mv   s1, a0
    la   t0, result_depth
    sw   s1, 0(t0)

    # Write the moves as text (e.g. "R B' D2\n") into solution_text
    li   s2, 0
    li   s3, 0
m_text_loop:
    bge  s3, s1, m_text_end
    la   t0, solution
    add  t0, t0, s3
    lbu  t1, 0(t0)
    slli t2, t1, 1
    add  t2, t2, t1
    la   t0, move_names
    add  t0, t0, t2
    beqz s3, m_name_start
    la   t1, solution_text
    add  t1, t1, s2
    li   t2, 32
    sb   t2, 0(t1)
    addi s2, s2, 1
m_name_start:
    li   t3, 0
m_name_loop:
    add  t4, t0, t3
    lbu  t4, 0(t4)
    beqz t4, m_name_end
    la   t1, solution_text
    add  t1, t1, s2
    sb   t4, 0(t1)
    addi s2, s2, 1
    addi t3, t3, 1
    j    m_name_loop
m_name_end:
    addi s3, s3, 1
    j    m_text_loop
m_text_end:
    la   t1, solution_text
    add  t1, t1, s2
    li   t2, 10
    sb   t2, 0(t1)
    addi s2, s2, 1
    la   t1, solution_text
    add  t1, t1, s2
    sb   zero, 0(t1)

    la   a0, solution_text    
    li   a7, 4                
    ecall                     
    
    # exit(moves)
    mv   a0, s1
    li   a7, 93
    ecall

# uint32_t rank_state(const state_t *state)   state_t: p at +0, o at +7
rank_state:
    addi sp, sp, -16
    sw   ra, 0(sp)
    # p = Lehmer code of state->p
    li   t0, 0
    li   t1, 0
    li   t2, 0
    li   t6, 7
rs_i_loop:
    bge  t2, t6, rs_o_start
    li   t3, 0
    addi t4, t2, 1
    add  a1, a0, t2
    lbu  a1, 0(a1)
rs_j_loop:
    bge  t4, t6, rs_j_end
    add  a2, a0, t4
    lbu  a2, 0(a2)
    bgeu a2, a1, rs_j_next
    addi t3, t3, 1
rs_j_next:
    addi t4, t4, 1
    j    rs_j_loop
rs_j_end:
    sub  a2, t6, t2
    sw   a0, 4(sp)
    sw   a1, 8(sp)
    mv   a0, t0
    mv   a1, a2
    jal  ra, __mulsi3
    mv   t0, a0
    lw   a1, 8(sp)
    lw   a0, 4(sp)
    add  t0, t0, t3
    addi t2, t2, 1
    j    rs_i_loop
rs_o_start:
    # o = base-3 number of state->o[0..5]
    li   t2, 0
    li   t6, 6
    li   a3, 3
rs_o_loop:
    bge  t2, t6, rs_end
    sw   a0, 4(sp)
    sw   a1, 8(sp)
    mv   a0, t1
    mv   a1, a3
    jal  ra, __mulsi3
    mv   t1, a0
    lw   a1, 8(sp)
    lw   a0, 4(sp)
    add  a2, a0, t2
    lbu  a2, 7(a2)
    add  t1, t1, a2
    addi t2, t2, 1
    j    rs_o_loop
rs_end:
    li   a3, 729
    sw   a0, 4(sp)
    sw   a1, 8(sp)
    mv   a0, t0
    mv   a1, a3
    jal  ra, __mulsi3
    mv   t0, a0
    lw   a1, 8(sp)
    lw   a0, 4(sp)
    add  a0, t0, t1
    lw   ra, 0(sp)
    addi sp, sp, 16
    ret

# uint8_t rank_c01(const state_t *state)
rank_c01:
    addi sp, sp, -16
    sw   ra, 0(sp)
    li   t0, 0
    li   t1, 0
    li   t2, 0
    li   t6, 7
    li   t5, 1
rc_loop:
    bge  t2, t6, rc_end
    add  t3, a0, t2
    lbu  t4, 0(t3)
    bnez t4, rc_check1
    lbu  t0, 7(t3)
rc_check1:
    bne  t4, t5, rc_next
    lbu  t1, 7(t3)
rc_next:
    addi t2, t2, 1
    j    rc_loop
rc_end:
    mv   a0, t0
    li   a1, 3
    jal  ra, __mulsi3
    mv   t3, a0
    add  a0, t3, t1
    andi a0, a0, 0xff
    lw   ra, 0(sp)
    addi sp, sp, 16
    ret

# uint32_t rank_po(const state_t *state)
rank_po:
    addi sp, sp, -16
    sw   ra, 12(sp)
    sw   s0, 8(sp)
    sw   s1, 4(sp)
    mv   s0, a0
    jal  ra, rank_state
    mv   a0, a0
    li   a1, 729
    jal  ra, __udivsi3
    mv   s1, a0
    mv   a0, s1
    li   a1, 9
    jal  ra, __mulsi3
    mv   s1, a0
    mv   a0, s0
    jal  ra, rank_c01
    add  a0, s1, a0
    lw   s1, 4(sp)
    lw   s0, 8(sp)
    lw   ra, 12(sp)
    addi sp, sp, 16
    ret

# int ida_search_bound(const state_t *root_state, int bound, int *next_bound)
#   s0 = root_state, s1 = d, s2 = min_f, s3 = bound, s4 = next_bound
#   s5 = face, s6 = turn, s7 = prev, s9 = rank, s10 = d + 1
ida_search_bound:
    addi sp, sp, -48
    sw   ra, 44(sp)
    sw   s0, 40(sp)
    sw   s1, 36(sp)
    sw   s2, 32(sp)
    sw   s3, 28(sp)
    sw   s4, 24(sp)
    sw   s5, 20(sp)
    sw   s6, 16(sp)
    sw   s7, 12(sp)
    sw   s9, 4(sp)
    sw   s10, 0(sp)
    mv   s0, a0
    mv   s3, a1
    mv   s4, a2
    li   s1, 0
    li   s2, 99

    # Root: st_p[0], st_o[0], st_cubie[0][0..1]
    mv   a0, s0
    jal  ra, rank_state
    mv   s9, a0
    mv   a0, s9
    li   a1, 729
    jal  ra, __udivsi3
    mv   t1, a0
    la   t2, st_p
    sh   t1, 0(t2)
    mv   a0, s9
    li   a1, 729
    jal  ra, __umodsi3
    mv   t1, a0
    la   t2, st_o
    sh   t1, 0(t2)

    li   t0, 0
    li   t6, 7
isb_root_loop:
    bge  t0, t6, isb_root_end
    add  t1, s0, t0
    lbu  t2, 0(t1)
    li   t3, 2
    bgeu t2, t3, isb_root_next
    lbu  t4, 7(t1)
    slli t5, t0, 2
    or   t5, t5, t4
    la   t3, st_cubie
    add  t3, t3, t2
    sb   t5, 0(t3)
isb_root_next:
    addi t0, t0, 1
    j    isb_root_loop
isb_root_end:
    la   t0, cur_face
    sb   zero, 0(t0)
    la   t0, cur_turn
    sb   zero, 0(t0)
    bnez s9, isb_while
    li   a0, 0
    j    isb_return

isb_while:
    blt  s1, zero, isb_exhausted
    la   t0, cur_face
    add  t0, t0, s1
    lbu  s5, 0(t0)
    li   t1, 3
    blt  s5, t1, isb_have_face
    addi s1, s1, -1
    j    isb_while
isb_have_face:
    la   t0, cur_turn
    add  t0, t0, s1
    lbu  s6, 0(t0)

    # Pruning: never turn the same face twice in a row
    bnez s6, isb_advance
    beqz s1, isb_advance
    la   t0, sol_face
    add  t0, t0, s1
    lbu  t1, -1(t0)
    bne  s5, t1, isb_advance
    la   t0, cur_face
    add  t0, t0, s1
    addi t1, s5, 1
    sb   t1, 0(t0)
    j    isb_while

isb_advance:
    # Advance to the next (face, turn) of this level
    li   t1, 2
    bne  s6, t1, isb_turn_inc
    la   t0, cur_face
    add  t0, t0, s1
    addi t1, s5, 1
    sb   t1, 0(t0)
    la   t0, cur_turn
    add  t0, t0, s1
    sb   zero, 0(t0)
    j    isb_apply
isb_turn_inc:                   # else
    la   t0, cur_turn
    add  t0, t0, s1
    addi t1, s6, 1
    sb   t1, 0(t0)

isb_apply:
    mv   s7, s1
    beqz s6, isb_prev_done
    addi s7, s1, 1
isb_prev_done:
    addi s10, s1, 1

    # st_p[d + 1] = permutation[face][st_p[prev]]
    la   t0, st_p
    slli t1, s7, 1
    add  t1, t0, t1
    lhu  t2, 0(t1)
    mv   a0, s5
    li   a1, 5040
    jal  ra, __mulsi3
    mv   t3, a0
    add  t3, t3, t2
    slli t3, t3, 1
    la   t4, permutation
    add  t4, t4, t3
    lhu  t5, 0(t4)
    slli t1, s10, 1
    add  t1, t0, t1
    sh   t5, 0(t1)

    # st_o[d + 1] = orientation[face][st_o[prev]]
    la   t0, st_o
    slli t1, s7, 1
    add  t1, t0, t1
    lhu  t2, 0(t1)
    mv   a0, s5
    li   a1, 729
    jal  ra, __mulsi3
    mv   t3, a0
    add  t3, t3, t2
    slli t3, t3, 1
    la   t4, orientation
    add  t4, t4, t3
    lhu  t5, 0(t4)
    slli t1, s10, 1
    add  t1, t0, t1
    sh   t5, 0(t1)

    # st_cubie[d + 1][0] = cubie_move[face][st_cubie[prev][0]]
    la   t0, st_cubie
    slli t1, s7, 1
    add  t1, t0, t1
    lbu  t2, 0(t1)
    slli t3, s5, 3
    sub  t3, t3, s5
    slli t3, t3, 2
    la   t4, cubie_move
    add  t4, t4, t3
    add  t4, t4, t2
    lbu  t5, 0(t4)
    slli t1, s10, 1
    add  t1, t0, t1
    sb   t5, 0(t1)

    # st_cubie[d + 1][1] = cubie_move[face][st_cubie[prev][1]]
    la   t0, st_cubie
    slli t1, s7, 1
    add  t1, t0, t1
    lbu  t2, 1(t1)
    slli t3, s5, 3
    sub  t3, t3, s5
    slli t3, t3, 2
    la   t4, cubie_move
    add  t4, t4, t3
    add  t4, t4, t2
    lbu  t5, 0(t4)
    slli t1, s10, 1
    add  t1, t0, t1
    sb   t5, 1(t1)

    # Combine two heuristics: dist_po (45,360) and dist_o (729)
    la   t0, st_p
    slli t1, s10, 1
    add  t1, t0, t1
    lhu  t2, 0(t1)
    la   t0, st_cubie
    slli t1, s10, 1
    add  t1, t0, t1
    lbu  t3, 0(t1)
    andi t3, t3, 3
    lbu  t4, 1(t1)
    andi t4, t4, 3
    slli t5, t2, 3
    add  t5, t5, t2
    slli t6, t3, 1
    add  t6, t6, t3
    add  t5, t5, t6
    add  t5, t5, t4
    la   t0, dist_po
    add  t0, t0, t5
    lbu  t1, 0(t0)
    la   t0, st_o
    slli t2, s10, 1
    add  t2, t0, t2
    lhu  t2, 0(t2)
    la   t0, dist_o
    add  t0, t0, t2
    lbu  t2, 0(t0)
    mv   t3, t2
    bge  t2, t1, isb_h_done
    mv   t3, t1
isb_h_done:
    add  t4, s10, t3

    # if (f > bound) { min_f = min(min_f, f); continue; }
    bge  s3, t4, isb_within
    bge  t4, s2, isb_while
    mv   s2, t4
    j    isb_while
isb_within:
    # solution[d] = face * 3 + turn; sol_face[d] = face
    slli t0, s5, 1
    add  t0, t0, s5
    add  t0, t0, s6
    la   t1, solution
    add  t1, t1, s1
    sb   t0, 0(t1)
    la   t1, sol_face
    add  t1, t1, s1
    sb   s5, 0(t1)

    # Check whether the cube is fully solved
    bnez t3, isb_descend
    mv   a0, s10
    j    isb_return
isb_descend:
    # d++; cur_face[d] = cur_turn[d] = 0
    mv   s1, s10
    la   t0, cur_face
    add  t0, t0, s1
    sb   zero, 0(t0)
    la   t0, cur_turn
    add  t0, t0, s1
    sb   zero, 0(t0)
    j    isb_while

isb_exhausted:
    # *next_bound = min_f; return -1
    sw   s2, 0(s4)
    li   a0, -1
isb_return:
    lw   s10, 0(sp)
    lw   s9, 4(sp)
    lw   s7, 12(sp)
    lw   s6, 16(sp)
    lw   s5, 20(sp)
    lw   s4, 24(sp)
    lw   s3, 28(sp)
    lw   s2, 32(sp)
    lw   s1, 36(sp)
    lw   s0, 40(sp)
    lw   ra, 44(sp)
    addi sp, sp, 48
    ret

# int solve_ida(const state_t *state)   s0 = state, s1 = h_po, s2 = bound
solve_ida:
    addi sp, sp, -32
    sw   ra, 28(sp)
    sw   s0, 24(sp)
    sw   s1, 20(sp)
    sw   s2, 16(sp)
    # bound = max(dist_po[rank_po(state)], dist_o[rank % 729])
    mv   s0, a0
    jal  ra, rank_po
    la   t0, dist_po
    add  t0, t0, a0
    lbu  s1, 0(t0)
    mv   a0, s0
    jal  ra, rank_state
    mv   a0, a0
    li   a1, 729
    jal  ra, __umodsi3
    mv   t0, a0
    la   t1, dist_o
    add  t1, t1, t0
    lbu  t1, 0(t1)
    mv   s2, t1
    bge  t1, s1, si_loop
    mv   s2, s1
si_loop:                        # for (;;)
    # Deepen the bound until ida_search_bound finds a solution
    mv   a0, s0
    mv   a1, s2
    addi a2, sp, 12
    jal  ra, ida_search_bound
    bge  a0, zero, si_return
    lw   t0, 12(sp)
    li   t1, 99
    blt  t0, t1, si_next
    li   a0, -1
    j    si_return
si_next:
    mv   s2, t0
    j    si_loop
si_return:
    lw   s2, 16(sp)
    lw   s1, 20(sp)
    lw   s0, 24(sp)
    lw   ra, 28(sp)
    addi sp, sp, 32
    ret

# ---- software multiply / divide: args a0, a1 -> a0, all other registers preserved

# uint32_t __mulsi3(uint32_t a, uint32_t b)       shift-and-add
__mulsi3:
    addi sp, sp, -16
    sw   t0, 0(sp)
    sw   t1, 4(sp)
    sw   t2, 8(sp)
    mv   t0, a0
    mv   t1, a1
    li   a0, 0
mul_loop:
    beqz t1, mul_done
    andi t2, t1, 1
    beqz t2, mul_skip
    add  a0, a0, t0
mul_skip:
    slli t0, t0, 1
    srli t1, t1, 1
    j    mul_loop
mul_done:
    lw   t2, 8(sp)
    lw   t1, 4(sp)
    lw   t0, 0(sp)
    addi sp, sp, 16
    ret

# uint32_t __udivsi3(uint32_t n, uint32_t d)      shift-and-subtract, quotient
__udivsi3:
    addi sp, sp, -20
    sw   t0, 0(sp)
    sw   t1, 4(sp)
    sw   t2, 8(sp)
    sw   t3, 12(sp)
    sw   t4, 16(sp)
    mv   t0, a0
    li   t1, 0
    li   t2, 0
    li   t3, 32
udiv_loop:
    beqz t3, udiv_done
    slli t2, t2, 1
    srli t4, t0, 31
    or   t2, t2, t4
    slli t0, t0, 1
    slli t1, t1, 1
    bltu t2, a1, udiv_next
    sub  t2, t2, a1
    ori  t1, t1, 1
udiv_next:
    addi t3, t3, -1
    j    udiv_loop
udiv_done:
    mv   a0, t1
    lw   t4, 16(sp)
    lw   t3, 12(sp)
    lw   t2, 8(sp)
    lw   t1, 4(sp)
    lw   t0, 0(sp)
    addi sp, sp, 20
    ret

# uint32_t __umodsi3(uint32_t n, uint32_t d)      shift-and-subtract, remainder
__umodsi3:
    addi sp, sp, -20
    sw   t0, 0(sp)
    sw   t1, 4(sp)
    sw   t2, 8(sp)
    sw   t3, 12(sp)
    sw   t4, 16(sp)
    mv   t0, a0
    li   t1, 0
    li   t2, 0
    li   t3, 32
umod_loop:
    beqz t3, umod_done
    slli t2, t2, 1
    srli t4, t0, 31
    or   t2, t2, t4
    slli t0, t0, 1
    slli t1, t1, 1
    bltu t2, a1, umod_next
    sub  t2, t2, a1
    ori  t1, t1, 1
umod_next:
    addi t3, t3, -1
    j    umod_loop
umod_done:
    mv   a0, t2
    lw   t4, 16(sp)
    lw   t3, 12(sp)
    lw   t2, 8(sp)
    lw   t1, 4(sp)
    lw   t0, 0(sp)
    addi sp, sp, 20
    ret

# ---- data (word -> halfword -> byte order keeps every array aligned)
    .data
result_depth:
    .word 0

st_p:                           # uint16_t st_p[16]
    .half 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
st_o:                           # uint16_t st_o[16]
    .half 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0

solution_text:                  # char solution_text[48], e.g. "R B' D2\n"
    .byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    .byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    .byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0

move_names:                     # static const char move_names[9][3]
    .byte  82,   0,   0          # "R"
    .byte  82,  50,   0          # "R2"
    .byte  82,  39,   0          # "R'"
    .byte  66,   0,   0          # "B"
    .byte  66,  50,   0          # "B2"
    .byte  66,  39,   0          # "B'"
    .byte  68,   0,   0          # "D"
    .byte  68,  50,   0          # "D2"
    .byte  68,  39,   0          # "D'"
    .byte 0                     # pad 27 -> 28 bytes

st_cubie:                       # uint8_t st_cubie[16][2]: (pos << 2) | ori
    .byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    .byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
cur_face:                       # uint8_t cur_face[16]
    .byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
cur_turn:                       # uint8_t cur_turn[16]
    .byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
solution:                       # uint8_t solution[16]
    .byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
sol_face:                       # uint8_t sol_face[16]
    .byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0

# Lookup tables (orientation, permutation, cubie_move, dist_o, dist_po) are in table.s
