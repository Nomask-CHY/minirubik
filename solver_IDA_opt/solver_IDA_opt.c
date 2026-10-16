#include <stdint.h>
#include <stdio.h>

/* Precomputed by build_table.c: dist_o, orientation, permutation, dist_po */
#include "tables.h"

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

static const char *const move_names[MOVES] = {
    "R",  "R2", "R'", 
    "B",  "B2", "B'", 
    "D",  "D2", "D'"
};

static const uint8_t source[3][CUBIES] = {
    {1, 4, 2, 0, 3, 5, 6},
    {0, 1, 2, 4, 5, 6, 3},
    {0, 2, 5, 3, 1, 4, 6},
};

static const uint8_t dest[3][CUBIES] = {
    {3, 0, 2, 4, 1, 5, 6},
    {0, 1, 2, 6, 3, 4, 5},
    {0, 4, 1, 3, 5, 2, 6}
};

static const uint8_t twist[3][CUBIES] = {
    {1, 2, 0, 2, 1, 0, 0},
    {0, 0, 0, 1, 2, 1, 2},
    {0, 0, 0, 0, 0, 0, 0},
};

static state_t quarter_turn(state_t state, uint8_t face)
{
    state_t result;
    for (uint8_t i = 0; i < CUBIES; ++i) {
        uint8_t from = source[face][i];
        result.p[i] = state.p[from];
        result.o[i] = (uint8_t)((state.o[from] + twist[face][i]) % 3U);
    }
    return result;
}

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

/* Orientation of cubie 0/1 after a turn: old orientation + twist at the new position (mod 3) */
static inline uint8_t moved_ori(uint8_t face, uint8_t ori, uint8_t new_pos)
{
    uint8_t c = (uint8_t)(ori + twist[face][new_pos]);
    return (c >= 3) ? (uint8_t)(c - 3) : c;
}

static uint16_t st_p[16];
static uint16_t st_o[16];
static uint8_t  st_pos[16][2];  /* positions of cubie 0 and cubie 1 */
static uint8_t  st_ori[16][2];  /* orientations of cubie 0 and cubie 1 */
static uint8_t  cur_face[16];
static uint8_t  cur_turn[16];
static uint8_t  solution[16];
static uint8_t  sol_face[16];

static int ida_search_bound(const state_t *root_state, int bound, int *next_bound)
{
    int d = 0;
    int min_f = 99;

    uint32_t rank = rank_state(root_state);
    st_p[0] = (uint16_t)(rank / ORIENTATIONS);
    st_o[0] = (uint16_t)(rank % ORIENTATIONS);
    for (uint8_t i = 0; i < CUBIES; ++i) {
        if (root_state->p[i] < 2) {
            st_pos[0][root_state->p[i]] = i;
            st_ori[0][root_state->p[i]] = root_state->o[i];
        }
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
        for (uint8_t k = 0; k < 2; ++k) {
            uint8_t pos = dest[face][st_pos[prev][k]];
            st_pos[d + 1][k] = pos;
            st_ori[d + 1][k] = moved_ori((uint8_t)face, st_ori[prev][k], pos);
        }

        /* Combine two heuristics: dist_po (45,360) and dist_o (729) */
        uint32_t p_val = st_p[d + 1];
        uint32_t o0    = st_ori[d + 1][0];
        uint32_t o1    = st_ori[d + 1][1];
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

        /* Check whether the cube is fully solved (all positions and all orientations) */
        if (h == 0)          /* 取代 st_p[d + 1] == 0 && st_o[d + 1] == 0 */
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
    static const struct {
        const char *name;
        const char *str;
        int expected_depth;
    } tests[] = {
        {"Solved Cube",    "12345671111111",  0},
        {"Short Scramble", "25713642221111",  8},
        {"Distance-11",    "21345671111111", 11}
    };

    int all_passed = 1;

    for (int t = 0; t < 3; ++t) {
        state_t state;
        for (int i = 0; i < 14; ++i)
            (i < 7 ? state.p : state.o)[i % 7] = (uint8_t)(tests[t].str[i] - '1');

        int moves = solve_ida(&state);

        int valid_depth = (moves == tests[t].expected_depth);

        state_t sim = state;
        for (int i = 0; i < moves; ++i) {
            uint8_t m = solution[i];
            uint8_t face = (uint8_t)(m / 3U);
            uint8_t turns = (uint8_t)(m % 3U + 1U);
            for (uint8_t k = 0; k < turns; ++k)
                sim = quarter_turn(sim, face);
        }
        int valid_solution = (rank_state(&sim) == 0);

        if (valid_depth && valid_solution) {
            printf("[PASS] %s (%d moves): ", tests[t].name, moves);
            for (int i = 0; i < moves; ++i)
                printf("%s%s", i ? " " : "", move_names[solution[i]]);
            putchar('\n');
        } else {
            printf("[FAIL] %s: got %d moves, verified=%d\n", tests[t].name, moves, valid_solution);
            all_passed = 0;
        }
    }

    return all_passed ? 0 : 1;
}