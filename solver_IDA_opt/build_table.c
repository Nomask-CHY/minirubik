#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    CUBIES = 7,
    PERMUTATIONS = 5040,
    ORIENTATIONS = 729,
    MOVES = 9,
    PO_STATES = 45360
};

typedef struct {
    uint8_t p[CUBIES], o[CUBIES];
} state_t;

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

static inline uint32_t rank_po(const state_t *state)
{
    uint8_t a = 0, b = 0;
    for (uint8_t i = 0; i < CUBIES; ++i) {
        if (state->p[i] == 0) a = state->o[i];
        if (state->p[i] == 1) b = state->o[i];
    }
    uint32_t p_rank = rank_state(state) / ORIENTATIONS;
    return p_rank * 9U + a * 3U + b;
}

static uint16_t orientation[3][ORIENTATIONS];
static uint16_t permutation[3][PERMUTATIONS];
static uint8_t dist_o[ORIENTATIONS];
static uint8_t dist_po[PO_STATES];

int main(void)
{
    state_t state;
    puts("Building Orientation Transition Table...");
    for (uint16_t rank = 0; rank < ORIENTATIONS; ++rank) {
        unrank_state(rank, &state);
        for (uint8_t face = 0; face < 3; ++face) {
            state_t next = quarter_turn(state, face);
            orientation[face][rank] = (uint16_t)(rank_state(&next) % ORIENTATIONS);
        }
    }

    puts("Building Permutation Transition Table...");
    for (uint16_t rank = 0; rank < PERMUTATIONS; ++rank) {
        unrank_state((uint32_t)rank * ORIENTATIONS, &state);
        for (uint8_t face = 0; face < 3; ++face) {
            state_t next = quarter_turn(state, face);
            permutation[face][rank] = (uint16_t)(rank_state(&next) / ORIENTATIONS);
        }
    }

    puts("Running BFS for dist_o (729 states)...");
    memset(dist_o, 0xFF, sizeof(dist_o));
    uint16_t q_o[ORIENTATIONS];
    int head = 0, tail = 0;
    dist_o[0] = 0;
    q_o[tail++] = 0;
    while (head < tail) {
        uint16_t curr = q_o[head++];
        for (uint8_t face = 0; face < 3; ++face) {
            uint16_t next = curr;
            for (uint8_t turn = 0; turn < 3; ++turn) {
                next = orientation[face][next];
                if (dist_o[next] == 0xFF) {
                    dist_o[next] = (uint8_t)(dist_o[curr] + 1);
                    q_o[tail++] = next;
                }
            }
        }
    }

    puts("Running BFS for dist_po (45,360 states)...");
    memset(dist_po, 0xFF, sizeof(dist_po));
    state_t *q_state = (state_t *)malloc(PO_STATES * sizeof(state_t));
    if (!q_state) {
        fprintf(stderr, "Out of memory allocating BFS queue!\n");
        return 1;
    }
    head = 0;
    tail = 0;

    const state_t solved = {{0, 1, 2, 3, 4, 5, 6}, {0}};
    uint32_t start_po = rank_po(&solved);
    dist_po[start_po] = 0;
    q_state[tail++] = solved;

    while (head < tail) {
        state_t curr = q_state[head++];
        uint32_t curr_po = rank_po(&curr);
        uint8_t cur_d = dist_po[curr_po];

        for (uint8_t face = 0; face < 3; ++face) {
            state_t next = curr;
            for (uint8_t turn = 0; turn < 3; ++turn) {
                next = quarter_turn(next, face);
                uint32_t next_po = rank_po(&next);
                if (dist_po[next_po] == 0xFF) {
                    dist_po[next_po] = (uint8_t)(cur_d + 1);
                    q_state[tail++] = next;
                }
            }
        }
    }
    free(q_state);

    puts("Exporting to tables.h...");
    FILE *fp = fopen("tables.h", "w");
    if (!fp) {
        perror("Failed to open tables.h");
        return 1;
    }

    fprintf(fp, "#ifndef TABLES_H\n#define TABLES_H\n\n#include <stdint.h>\n\n");

    /* 1. dist_o */
    fprintf(fp, "static const uint8_t dist_o[%d] = {\n", ORIENTATIONS);
    for (int i = 0; i < ORIENTATIONS; ++i) {
        fprintf(fp, "%u, ", dist_o[i]);
        if ((i + 1) % 24 == 0) fprintf(fp, "\n");
    }
    fprintf(fp, "\n};\n\n");

    /* 2. orientation */
    fprintf(fp, "static const uint16_t orientation[3][%d] = {\n", ORIENTATIONS);
    for (int face = 0; face < 3; ++face) {
        fprintf(fp, "  {\n");
        for (int i = 0; i < ORIENTATIONS; ++i) {
            fprintf(fp, "%u, ", orientation[face][i]);
            if ((i + 1) % 20 == 0) fprintf(fp, "\n");
        }
        fprintf(fp, "\n  },\n");
    }
    fprintf(fp, "};\n\n");

    /* 3. permutation */
    fprintf(fp, "static const uint16_t permutation[3][%d] = {\n", PERMUTATIONS);
    for (int face = 0; face < 3; ++face) {
        fprintf(fp, "  {\n");
        for (int i = 0; i < PERMUTATIONS; ++i) {
            fprintf(fp, "%u, ", permutation[face][i]);
            if ((i + 1) % 20 == 0) fprintf(fp, "\n");
        }
        fprintf(fp, "\n  },\n");
    }
    fprintf(fp, "};\n\n");

    /* 4. dist_po */
    fprintf(fp, "static const uint8_t dist_po[%d] = {\n", PO_STATES);
    for (int i = 0; i < PO_STATES; ++i) {
        fprintf(fp, "%u, ", dist_po[i]);
        if ((i + 1) % 24 == 0) fprintf(fp, "\n");
    }
    fprintf(fp, "\n};\n\n#endif /* TABLES_H */\n");

    fclose(fp);
    puts("Done! tables.h generated successfully.");
    return 0;
}