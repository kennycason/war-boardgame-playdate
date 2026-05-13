#include <stdlib.h>
#include <string.h>
#include "ai.h"
#include "piece.h"

#define INF_SCORE 1000000

static int g_nodes = 0;

int ai_last_node_count(void) { return g_nodes; }

/* ---- Evaluation ----
 * Material + positional terms. Tuned to encourage AI to push pieces toward
 * the enemy commander and protect its own. */

static int distance(int ax, int ay, int bx, int by) {
    int dx = ax - bx; if (dx < 0) dx = -dx;
    int dy = ay - by; if (dy < 0) dy = -dy;
    return dx > dy ? dx : dy; /* Chebyshev — most pieces move 8-directionally */
}

int ai_evaluate(const Board* b, Player player) {
    Player enemy = (player == PLAYER_BLACK) ? PLAYER_WHITE : PLAYER_BLACK;

    int my_material = 0;
    int op_material = 0;
    int my_cmd_x = -1, my_cmd_y = -1;
    int op_cmd_x = -1, op_cmd_y = -1;

    for (int x = 0; x < BOARD_DIM; x++) {
        for (int y = 0; y < BOARD_DIM; y++) {
            const Piece* p = &b->tiles[x][y].piece;
            if (p->type == PIECE_NONE) continue;
            int v = piece_score(p->type);
            if (p->player == player) {
                my_material += v;
                if (p->type == PIECE_COMMANDER) { my_cmd_x = x; my_cmd_y = y; }
            } else if (p->player == enemy) {
                op_material += v;
                if (p->type == PIECE_COMMANDER) { op_cmd_x = x; op_cmd_y = y; }
            }
        }
    }

    /* Terminal preference: capturing the commander dominates. */
    if (op_cmd_x < 0) return  INF_SCORE - 1;
    if (my_cmd_x < 0) return -INF_SCORE + 1;

    int score = (my_material - op_material) * 10;

    /* Encourage pieces to advance toward the enemy commander.
     * Threat tax: enemies near MY commander are bad. */
    for (int x = 0; x < BOARD_DIM; x++) {
        for (int y = 0; y < BOARD_DIM; y++) {
            const Piece* p = &b->tiles[x][y].piece;
            if (p->type == PIECE_NONE || p->type == PIECE_COMMANDER) continue;
            if (p->player == player) {
                int d = distance(x, y, op_cmd_x, op_cmd_y);
                score += (BOARD_DIM - d); /* closer to enemy commander = better */
            } else {
                int d = distance(x, y, my_cmd_x, my_cmd_y);
                score -= (BOARD_DIM - d);
            }
        }
    }

    /* Score difference baked in (since apply_move adds to it). */
    if (player == PLAYER_BLACK) score += (b->black_score - b->white_score) * 5;
    else                        score += (b->white_score - b->black_score) * 5;

    return score;
}

/* ---- Move ordering ----
 * Sort attacks before plain moves; among attacks, sort by destroyed value. */
static int move_order_key(const Move* m) {
    int k = 0;
    if (m->move_type == MOVE_TYPE_ATTACK) {
        k += 1000 + piece_score(m->destroyed_type) * 10;
    }
    return k;
}

static int cmp_moves_desc(const void* a, const void* b) {
    return move_order_key((const Move*)b) - move_order_key((const Move*)a);
}

/* ---- Undo support ----
 * apply_move isn't symmetric for special pieces (artillery reload, missile
 * self-destruct, bomber/AD interception, excavator). Cheapest correct undo is
 * to snapshot the board before each ply and restore. */

typedef struct {
    Board snapshot;
} Undo;

static void save(Undo* u, const Board* b) { u->snapshot = *b; }
static void load(const Undo* u, Board* b) { *b = u->snapshot; }

/* ---- Minimax with alpha-beta ---- */

/* Score from the perspective of the side currently to move. */
static int negamax(Board* b, int depth, int alpha, int beta) {
    g_nodes++;

    if (b->win_state != WIN_NONE) {
        Player me = b->current_player;
        if ((b->win_state == WIN_BLACK && me == PLAYER_BLACK)
            || (b->win_state == WIN_WHITE && me == PLAYER_WHITE)) {
            return  INF_SCORE - 1;
        }
        if (b->win_state == WIN_DRAW) return 0;
        return -INF_SCORE + 1;
    }
    if (depth == 0) {
        return ai_evaluate(b, b->current_player);
    }

    Move moves[BOARD_DIM * BOARD_DIM * 4];
    int n = 0;
    for (int x = 0; x < BOARD_DIM && n < (int)(sizeof(moves)/sizeof(moves[0])); x++) {
        for (int y = 0; y < BOARD_DIM && n < (int)(sizeof(moves)/sizeof(moves[0])); y++) {
            const Piece* p = &b->tiles[x][y].piece;
            if (p->type == PIECE_NONE) continue;
            if (p->player != b->current_player) continue;
            int room = (int)(sizeof(moves)/sizeof(moves[0])) - n;
            int got = generate_moves(b, x, y, &moves[n], room);
            n += got;
        }
    }
    if (n == 0) return -INF_SCORE + 1;

    qsort(moves, n, sizeof(Move), cmp_moves_desc);

    int best = -INF_SCORE;
    Undo u;
    for (int i = 0; i < n; i++) {
        save(&u, b);
        apply_move(b, &moves[i]);
        int score = -negamax(b, depth - 1, -beta, -alpha);
        load(&u, b);

        if (score > best) best = score;
        if (best > alpha) alpha = best;
        if (alpha >= beta) break;
    }
    return best;
}

/* Level → search depth and "noise" percent.
 * Noise = probability we pick a non-best move from the top candidates instead
 * of the best. Depth 3 is borderline-slow on Playdate; the higher levels keep
 * it but tighten the search by removing noise. */
void ai_level_config(int level, int* out_depth, int* out_noise_pct) {
    if (level < AI_LEVEL_MIN) level = AI_LEVEL_MIN;
    if (level > AI_LEVEL_MAX) level = AI_LEVEL_MAX;
    switch (level) {
    case 1: *out_depth = 1; *out_noise_pct = 70; break;
    case 2: *out_depth = 1; *out_noise_pct = 40; break;
    case 3: *out_depth = 1; *out_noise_pct = 0;  break;
    case 4: *out_depth = 2; *out_noise_pct = 50; break;
    case 5: *out_depth = 2; *out_noise_pct = 25; break;
    case 6: *out_depth = 2; *out_noise_pct = 0;  break;
    case 7: *out_depth = 3; *out_noise_pct = 40; break;
    case 8: *out_depth = 3; *out_noise_pct = 20; break;
    case 9: *out_depth = 3; *out_noise_pct = 0;  break;
    default:*out_depth = 2; *out_noise_pct = 0;  break;
    }
}

/* Helpers: cmp by precomputed score descending — used to sort root moves after
 * we know their α-β values. */
static int  g_root_scores[BOARD_DIM * BOARD_DIM * 4];

static int cmp_indices_by_score_desc(const void* a, const void* b) {
    int ia = *(const int*)a, ib = *(const int*)b;
    return g_root_scores[ib] - g_root_scores[ia];
}

bool ai_pick_move_at_depth(Board* b, int depth, int noise_pct, Move* out) {
    g_nodes = 0;

    Player root = b->current_player;

    /* Collect root moves. */
    Move moves[BOARD_DIM * BOARD_DIM * 4];
    int n = 0;
    for (int x = 0; x < BOARD_DIM && n < (int)(sizeof(moves)/sizeof(moves[0])); x++) {
        for (int y = 0; y < BOARD_DIM && n < (int)(sizeof(moves)/sizeof(moves[0])); y++) {
            const Piece* p = &b->tiles[x][y].piece;
            if (p->type == PIECE_NONE || p->player != root) continue;
            int room = (int)(sizeof(moves)/sizeof(moves[0])) - n;
            int got = generate_moves(b, x, y, &moves[n], room);
            n += got;
        }
    }
    if (n == 0) return false;

    qsort(moves, n, sizeof(Move), cmp_moves_desc);

    int  alpha = -INF_SCORE;
    int  beta  =  INF_SCORE;
    Undo u;

    /* Score every root move so we can also rank for noise. */
    for (int i = 0; i < n; i++) {
        save(&u, b);
        apply_move(b, &moves[i]);
        int score = -negamax(b, depth - 1, -beta, -alpha);
        load(&u, b);
        g_root_scores[i] = score;
        if (score > alpha) alpha = score;
    }

    /* Sort indices by score descending. */
    int idx[BOARD_DIM * BOARD_DIM * 4];
    for (int i = 0; i < n; i++) idx[i] = i;
    qsort(idx, n, sizeof(int), cmp_indices_by_score_desc);

    int chosen = idx[0];
    if (noise_pct > 0 && (rand() % 100) < noise_pct) {
        /* Pick from the top 3 (or fewer if fewer moves). Skip clearly losing
         * moves (anything < best - 20 — about a piece-and-a-half). */
        int pool = (n < 3) ? n : 3;
        int best_score = g_root_scores[idx[0]];
        int real_pool = 0;
        for (int i = 0; i < pool; i++) {
            if (g_root_scores[idx[i]] >= best_score - 20) real_pool++;
            else break;
        }
        if (real_pool < 1) real_pool = 1;
        chosen = idx[rand() % real_pool];
    }
    (void)root;
    *out = moves[chosen];
    return true;
}

bool ai_pick_move(Board* b, int level, Move* out) {
    int depth, noise_pct;
    ai_level_config(level, &depth, &noise_pct);
    return ai_pick_move_at_depth(b, depth, noise_pct, out);
}
