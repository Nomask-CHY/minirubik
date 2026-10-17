#include <stdint.h>

/* Precomputed by build_table.c: dist_o, orientation, permutation, cubie_move, dist_po */
#include "tables.h"

/*
 * Test case: edit this line to change the input.
 * test_str = p[0..6] then o[0..6], 1-based (14 characters)
 * Other cases (optimal length):
 *   "12345671111111"  0   (solved cube)
 *   "25713642221111"  8   (short scramble)
 *   "21345671111111" 11   (distance-11)
 */
static const char test_str[] = "21345671111111";

enum {
    CUBIES = 7,
    PERMUTATIONS = 5040,
    ORIENTATIONS = 729,
    MOVES = 9,
    /* 5040 permutations * 3^2 (orientations of cubie 0 & cubie 1) = 45360 */
    PO_STATES = 45360
};

typedef struct {
    uint8_t p[CUBIES], o[CUBIES];
} state_t;

static const char move_names[MOVES][3] = {
    "R",  "R2", "R'",
    "B",  "B2", "B'",
    "D",  "D2", "D'"
};

static uint32_t rank_state(const state_t *state)
{
    uint32_t p = 0, o = 0;
    for (uint8_t i = 0; i < CUBIES; ++i) {
        uint8_t smaller = 0;
        for (uint8_t j = (uint8_t)(i + 1U); j < CUBIES; ++j) {
            if (state->p[j] < state->p[i])
                ++smaller;
        }
        p = p * (CUBIES - i) + smaller;
    }
    for (uint8_t i = 0; i < 6; ++i)
        o = o * 3U + state->o[i];
    return p * ORIENTATIONS + o;
}

/* Orientations of cubie 0 and cubie 1 (wherever they are), encoded as 0~8 */
static inline uint8_t rank_c01(const state_t *state)
{
    uint8_t a = 0, b = 0;
    for (uint8_t i = 0; i < CUBIES; ++i) {
        if (state->p[i] == 0) a = state->o[i];
        if (state->p[i] == 1) b = state->o[i];
    }
    return (uint8_t)(a * 3U + b);
}

/* Joint index of the full permutation and the orientations of cubie 0 & cubie 1 */
static inline uint32_t rank_po(const state_t *state)
{
    uint32_t p_rank = rank_state(state) / ORIENTATIONS;
    return p_rank * 9U + rank_c01(state);
}

static uint16_t st_p[16];
static uint16_t st_o[16];
static uint8_t  st_cubie[16][2];    /* (pos << 2) | ori of cubie 0 and cubie 1 */
static uint8_t  cur_face[16];
static uint8_t  cur_turn[16];
static uint8_t  solution[16];
static uint8_t  sol_face[16];

/* Global debug variables: can be inspected directly in the Ripes Memory view */
volatile int result_depth;
char solution_text[48];             /* e.g. "R B' D2\n" */

static int ida_search_bound(const state_t *root_state, int bound, int *next_bound)
{
    int d = 0;
    int min_f = 99;

    uint32_t rank = rank_state(root_state);
    st_p[0] = (uint16_t)(rank / ORIENTATIONS);
    st_o[0] = (uint16_t)(rank % ORIENTATIONS);
    for (uint8_t i = 0; i < CUBIES; ++i) {
        if (root_state->p[i] < 2)
            st_cubie[0][root_state->p[i]] = (uint8_t)((i << 2) | root_state->o[i]);
    }
    cur_face[0] = 0;
    cur_turn[0] = 0;

    if (rank == 0)
        return 0;

    while (d >= 0) {
        int face = cur_face[d];
        if (face >= 3) {
            d--;
            continue;
        }

        int turn = cur_turn[d];

        /* Pruning: never turn the same face twice in a row */
        if (turn == 0 && d > 0 && face == sol_face[d - 1]) {
            cur_face[d]++;
            continue;
        }

        if (turn == 2) {
            cur_face[d]++;
            cur_turn[d] = 0;
        } else {
            cur_turn[d]++;
        }

        int prev = (turn == 0) ? d : d + 1;
        st_p[d + 1] = permutation[face][st_p[prev]];
        st_o[d + 1] = orientation[face][st_o[prev]];
        st_cubie[d + 1][0] = cubie_move[face][st_cubie[prev][0]];
        st_cubie[d + 1][1] = cubie_move[face][st_cubie[prev][1]];

        /* Combine two heuristics: dist_po (45,360) and dist_o (729) */
        uint32_t p_val = st_p[d + 1];
        uint32_t o0    = st_cubie[d + 1][0] & 3U;
        uint32_t o1    = st_cubie[d + 1][1] & 3U;
        uint32_t po_idx = ((p_val << 3) + p_val) + ((o0 << 1) + o0) + o1;
        int h_po = dist_po[po_idx];
        int h_o  = dist_o[st_o[d + 1]];
        int h = (h_po > h_o) ? h_po : h_o;
        int f = (d + 1) + h;

        if (f > bound) {
            if (f < min_f) min_f = f;
            continue;
        }

        solution[d] = (uint8_t)((face << 1) + face + turn);
        sol_face[d] = (uint8_t)face;

        /* Check whether the cube is fully solved */
        if (h == 0)
            return d + 1;

        d++;
        cur_face[d] = 0;
        cur_turn[d] = 0;
    }

    *next_bound = min_f;
    return -1;
}

static int solve_ida(const state_t *state)
{
    int h_po = dist_po[rank_po(state)];
    int h_o  = dist_o[rank_state(state) % ORIENTATIONS];
    int bound = (h_po > h_o) ? h_po : h_o;

    for (;;) {
        int next_bound;
        int depth = ida_search_bound(state, bound, &next_bound);
        if (depth >= 0)
            return depth;
        if (next_bound >= 99)
            return -1;
        bound = next_bound;
    }
}

int main(void)
{
    state_t state;
    for (int i = 0; i < 14; ++i)
        (i < 7 ? state.p : state.o)[i % 7] = (uint8_t)(test_str[i] - '1');

    int moves = solve_ida(&state);
    result_depth = moves;

    /* Write the moves as text (e.g. "R B' D2\n") into solution_text */
    int n = 0;
    for (int i = 0; i < moves; ++i) {
        const char *name = move_names[solution[i]];
        if (i > 0)
            solution_text[n++] = ' ';
        for (int j = 0; name[j] != '\0'; ++j)
            solution_text[n++] = name[j];
    }
    solution_text[n++] = '\n';
    solution_text[n] = '\0';

    /* a0 = number of moves (-1 if no solution) */
    return moves;
}
