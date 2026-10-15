#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    CUBIES = 7,
    PERMUTATIONS = 5040,
    ORIENTATIONS = 729,
    STATES = 3674160,
    MOVES = 9
};

typedef struct {
    uint8_t p[CUBIES], o[CUBIES];
} state_t;

static const char *const move_names[MOVES] = {
    "R",  "R2", "R'", 
    "B",  "B2", "B'", 
    "D",  "D2", "D'"
};

static const uint8_t inverse_move[MOVES] = {2, 1, 0, 5, 4, 3, 8, 7, 6};

static const uint8_t source[3][CUBIES] = {
    {1, 4, 2, 0, 3, 5, 6},
    {0, 1, 2, 4, 5, 6, 3},
    {0, 2, 5, 3, 1, 4, 6},
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

static state_t apply_move(state_t state, uint8_t move)
{
    uint8_t face = (uint8_t)(move / 3U);
    uint8_t turns = (uint8_t)(move % 3U + 1U);
    for (uint8_t i = 0; i < turns; ++i)
        state = quarter_turn(state, face);
    return state;
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

static void unrank_state(uint32_t rank, state_t *state)
{
    uint8_t available[CUBIES] = {0, 1, 2, 3, 4, 5, 6};
    uint32_t p = rank / ORIENTATIONS, o = rank % ORIENTATIONS, f = 720;
    uint8_t sum = 0;
    for (uint8_t i = 0; i < CUBIES; ++i) {
        uint8_t q = (uint8_t)(p / f);
        p %= f;
        state->p[i] = available[q];
        for (uint8_t j = q; j + 1U < CUBIES - i; ++j)
            available[j] = available[j + 1U];
        if (i < 5)
            f /= 6U - i;
    }
    for (uint8_t i = 6; i-- > 0;) {
        state->o[i] = (uint8_t)(o % 3U);
        sum = (uint8_t)(sum + state->o[i]);
        o /= 3U;
    }
    state->o[6] = (uint8_t)((3U - sum % 3U) % 3U);
}

static uint16_t permutation[3][PERMUTATIONS];
static uint16_t orientation[3][ORIENTATIONS];

// distance tables for IDA* heuristic
static uint8_t dist_p[PERMUTATIONS];
static size_t table_bytes = 0;

static void init_tables_and_pdb(void)
{
    state_t state;

    for (uint16_t rank = 0; rank < PERMUTATIONS; ++rank) {
        unrank_state((uint32_t)rank * ORIENTATIONS, &state);
        for (uint8_t face = 0; face < 3; ++face) {
            state_t next = quarter_turn(state, face);
            permutation[face][rank] = (uint16_t)(rank_state(&next) / ORIENTATIONS);
        }
    }

    for (uint16_t rank = 0; rank < ORIENTATIONS; ++rank) {
        unrank_state(rank, &state);
        for (uint8_t face = 0; face < 3; ++face) {
            state_t next = quarter_turn(state, face);
            orientation[face][rank] =
                (uint16_t) (rank_state(&next) % ORIENTATIONS);
        }
    }

    /* Run BFS on permutation graph to build PDB dist_p (5040 states) */
    memset(dist_p, 0xFF, sizeof(dist_p));
    uint16_t q_p[PERMUTATIONS];
    int head = 0, tail = 0;
    dist_p[0] = 0;
    q_p[tail++] = 0;
    while (head < tail) {
        uint16_t curr = q_p[head++];
        for (uint8_t face = 0; face < 3; ++face) {
            uint16_t next = curr;
            for (uint8_t turn = 0; turn < 3; ++turn) {
                next = permutation[face][next];
                if (dist_p[next] == 0xFF) {
                    dist_p[next] = (uint8_t)(dist_p[curr] + 1);
                    q_p[tail++] = next;
                }
            }
        }
    }

    table_bytes = sizeof(permutation) + sizeof(orientation) + sizeof(dist_p);
}

static uint8_t *build_table(uint8_t *diameter)
{
    init_tables_and_pdb();

    uint8_t max_d = 0;
    uint16_t count = 0;
    for (uint16_t i = 0; i < PERMUTATIONS; ++i) {
        if (dist_p[i] != 0xFF) {
            count++;
            if (dist_p[i] > max_d)
                max_d = dist_p[i];
        }
    }

    if (count != PERMUTATIONS)
        return NULL;

    *diameter = max_d;
    return dist_p; 
}

static uint16_t st_p[16];
static uint16_t st_o[16];
static uint8_t  cur_face[16];
static uint8_t  cur_turn[16];
static uint8_t  solution[16];
static unsigned long nodes_visited;

static int ida_search_bound(uint16_t p_root, uint16_t o_root, int bound, int *next_bound)
{
    int d = 0;
    int min_f = 99;

    st_p[0] = p_root;
    st_o[0] = o_root;
    cur_face[0] = 0;
    cur_turn[0] = 0;

    if (p_root == 0 && o_root == 0)
        return 0;

    while (d >= 0) {
        int face = cur_face[d];
        if (face >= 3) {
            d--; /* Backtrack: all 3 faces exhausted at this depth */
            continue;
        }

        int turn = cur_turn[d];

        /* Move pruning: reject redundant consecutive moves on the same face.
         * The check occurs at turn == 0 before evaluating any rotation angle. */
        if (turn == 0 && d > 0 && face == (solution[d - 1] / 3)) {
            cur_face[d]++;
            continue;
        }

        /* Advance move generator cursor for next visit or backtracking */
        if (turn == 2) {
            cur_face[d]++;
            cur_turn[d] = 0;
        } else {
            cur_turn[d]++;
        }

        uint16_t prev_p = (turn == 0) ? st_p[d] : st_p[d + 1];
        uint16_t prev_o = (turn == 0) ? st_o[d] : st_o[d + 1];

        st_p[d + 1] = permutation[face][prev_p];
        st_o[d + 1] = orientation[face][prev_o];

        nodes_visited++;
        /* Heuristic evaluation: relaxed permutation-only distance table */
        int h = dist_p[st_p[d + 1]];
        int f = (d + 1) + h;

        if (f > bound) {
            if (f < min_f) min_f = f;
            continue; /* pruned */
        }

        solution[d] = (uint8_t)(face * 3 + turn);

        /* 抵達目標狀態 */
        if (st_p[d + 1] == 0 && st_o[d + 1] == 0)
            return d + 1;

        /* 深入下一層 */
        d++;
        cur_face[d] = 0;
        cur_turn[d] = 0;
    }

    *next_bound = min_f;
    return -1;
}

static int solve_ida(uint16_t p, uint16_t o)
{
    int bound = dist_p[p];
    nodes_visited = 0;

    for (;;) {
        int next_bound;
        int depth = ida_search_bound(p, o, bound, &next_bound);
        if (depth >= 0)
            return depth;
        if (next_bound >= 99)
            return -1; /* 無解 */
        bound = next_bound;
    }
}

static int valid(const state_t *state)
{
    uint8_t sum = 0;
    for (uint8_t i = 0; i < CUBIES; ++i) {
        if (state->p[i] >= CUBIES || state->o[i] >= 3)
            return 0;
        for (uint8_t j = 0; j < i; ++j) {
            if (state->p[j] == state->p[i])
                return 0;
        }
        sum = (uint8_t)(sum + state->o[i]);
    }
    return (sum % 3U) == 0;
}

static int parse_state(const char *input, state_t *state)
{
    for (int i = 0; i < 14; ++i) {
        int limit = (i < 7) ? 7 : 3;
        if (input[i] < '1' || input[i] > '0' + limit)
            return 0;
        (i < 7 ? state->p : state->o)[i % 7] = (uint8_t)(input[i] - '1');
    }
    return (input[14] == '\0') && valid(state);
}

static int self_test(void)
{
    const state_t solved = {{0, 1, 2, 3, 4, 5, 6}, {0}};
    state_t state;
    for (uint8_t move = 0; move < MOVES; ++move) {
        state = solved;
        state = apply_move(state, move);
        state = apply_move(state, inverse_move[move]);
        if (memcmp(&solved, &state, sizeof solved))
            return 0;
    }
    for (uint32_t rank = 0; rank < STATES; ++rank) {
        unrank_state(rank, &state);
        if (!valid(&state) || rank_state(&state) != rank)
            return 0;
    }
    return 1;
}

int main(int argc, char **argv)
{
    state_t state;
    uint8_t diameter;

    if (argc == 2 && !strcmp(argv[1], "--self-test")) {
        if (!self_test()) {
            fputs("self-test failed\n", stderr);
            return 1;
        }
        uint8_t *table = build_table(&diameter);
        if (!table) {
            fputs("could not build complete state table\n", stderr);
            return 1;
        }
        if (diameter != 7) {
            fputs("BFS check failed\n", stderr);
            return 1;
        }
        printf("%d states; diameter %u; table size: %zu bytes\n",
               PERMUTATIONS, diameter, table_bytes);
        return 0;
    }

    if (argc != 2 || !parse_state(argv[1], &state)) {
        fprintf(stderr, "usage: %s PPPPPPPOOOOOOO\n",
                argc > 0 && argv[0] ? argv[0] : "solver");
        return 2;
    }

    uint8_t *table = build_table(&diameter);
    if (!table) {
        fputs("could not build complete state table\n", stderr);
        return 1;
    }

    uint32_t rank = rank_state(&state);
    uint16_t p = (uint16_t)(rank / ORIENTATIONS);
    uint16_t o = (uint16_t)(rank % ORIENTATIONS);

    int moves = solve_ida(p, o);
    if (moves < 0) {
        puts("unsolvable");
        return 1;
    }

    const char *separator = "";
    for (int i = 0; i < moves; ++i) {
        printf("%s%s", separator, move_names[solution[i]]);
        separator = " ";
    }
    putchar('\n');

    fprintf(stderr, "Solved in %d moves, nodes expanded: %lu\n", moves, nodes_visited);
    return 0;
}