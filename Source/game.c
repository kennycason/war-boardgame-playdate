#include <string.h>
#include "game.h"
#include "ai.h"

static int clamp(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static void clear_selection(GameState* g) {
    g->selected_x      = -1;
    g->selected_y      = -1;
    g->num_valid_moves = 0;
    if (g->board.win_state != WIN_NONE) g->mode = GAME_MODE_GAME_OVER;
    else if (g->mode == GAME_MODE_PIECE_SELECTED) g->mode = GAME_MODE_FREE;
    /* Refresh (don't just clear) so the new mode's hover preview kicks in. */
    game_refresh_highlights(g);
}

static void select_piece(GameState* g, int x, int y) {
    g->selected_x      = x;
    g->selected_y      = y;
    g->num_valid_moves = generate_moves(&g->board, x, y, g->valid_moves, MAX_MOVES_PER_PIECE);
    g->mode            = GAME_MODE_PIECE_SELECTED;
    game_refresh_highlights(g);
}

/* Mark all of the piece-at-(sx,sy)'s legal destinations with MOVE/ATTACK
 * highlights. Caller is responsible for clearing/setting SELECTED. */
static void mark_piece_moves(GameState* g, int sx, int sy) {
    const Piece* p = &g->board.tiles[sx][sy].piece;
    if (p->type == PIECE_NONE) return;
    Move scratch[MAX_MOVES_PER_PIECE];
    int n = generate_moves(&g->board, sx, sy, scratch, MAX_MOVES_PER_PIECE);
    for (int i = 0; i < n; i++) {
        const Move* m = &scratch[i];
        TileHighlight h = (m->move_type == MOVE_TYPE_ATTACK) ? HIGHLIGHT_ATTACK : HIGHLIGHT_MOVE;
        if (g->board.tiles[m->to_x][m->to_y].highlight != HIGHLIGHT_SELECTED) {
            g->board.tiles[m->to_x][m->to_y].highlight = h;
        }
    }
}

void game_refresh_highlights(GameState* g) {
    board_clear_highlights(&g->board);

    if (g->mode == GAME_MODE_PIECE_SELECTED) {
        g->board.tiles[g->selected_x][g->selected_y].highlight = HIGHLIGHT_SELECTED;
        for (int i = 0; i < g->num_valid_moves; i++) {
            const Move* m = &g->valid_moves[i];
            TileHighlight h = (m->move_type == MOVE_TYPE_ATTACK) ? HIGHLIGHT_ATTACK : HIGHLIGHT_MOVE;
            if (g->board.tiles[m->to_x][m->to_y].highlight != HIGHLIGHT_SELECTED) {
                g->board.tiles[m->to_x][m->to_y].highlight = h;
            }
        }
        return;
    }

    /* FREE mode hover preview — works for own pieces and enemies alike so
     * the player can read threats without selecting. The hovered piece's
     * tile also gets the SELECTED frame so the visual is consistent with
     * an actively-clicked selection. */
    if (g->mode == GAME_MODE_FREE) {
        const Piece* p = &g->board.tiles[g->cursor_x][g->cursor_y].piece;
        if (p->type != PIECE_NONE) {
            g->board.tiles[g->cursor_x][g->cursor_y].highlight = HIGHLIGHT_SELECTED;
        }
        mark_piece_moves(g, g->cursor_x, g->cursor_y);
    }
}

bool game_is_valid_destination(const GameState* g, int x, int y, Move* out_move) {
    for (int i = 0; i < g->num_valid_moves; i++) {
        if (g->valid_moves[i].to_x == x && g->valid_moves[i].to_y == y) {
            if (out_move) *out_move = g->valid_moves[i];
            return true;
        }
    }
    return false;
}

static bool current_player_is_ai(const GameState* g) {
    return g->settings.match == MATCH_PVA
        && g->board.current_player == g->settings.ai_player
        && g->board.win_state == WIN_NONE;
}

static void after_move_applied(GameState* g, const Move* applied) {
    g->last_move     = *applied;
    g->has_last_move = true;
    g->flash_counter = 12;
    history_push(&g->history, &g->board, applied);

    if (g->board.win_state != WIN_NONE) {
        g->mode = GAME_MODE_GAME_OVER;
    } else if (current_player_is_ai(g)) {
        g->mode = GAME_MODE_AI_THINKING;
        g->ai_pending = true;
        g->ai_thinking_frames = 0;
    } else {
        g->mode = GAME_MODE_FREE;
    }
}

void game_init(GameState* g) {
    memset(g, 0, sizeof(GameState));
    g->settings.match     = MATCH_PVP;
    g->settings.ai_level  = 5;
    g->settings.ai_player = PLAYER_WHITE;
    g->settings.render    = RENDER_ELEVATED;
    g->settings.map       = MAP_DEFAULT;
    g->mode                   = GAME_MODE_TITLE;
    g->menu_row               = 0;
    g->selected_x             = -1;
    g->selected_y             = -1;
}

void game_reset(GameState* g) {
    board_reset(&g->board, g->settings.map);
    g->mode               = GAME_MODE_FREE;
    /* Start on an empty tile so the initial frame has no hover preview —
     * the player explicitly moves the cursor onto a piece to see its moves. */
    g->cursor_x           = 5;
    g->cursor_y           = 4;
    g->selected_x         = -1;
    g->selected_y         = -1;
    g->num_valid_moves    = 0;
    g->has_last_move      = false;
    g->flash_counter      = 0;
    g->ai_pending         = false;
    g->ai_thinking_frames = 0;
    g->ai_next_depth      = 0;
    g->ai_has_best        = false;
    g->review_offset      = 0;
    g->review_crank_accum = 0.0f;
    g->a_held             = false;
    g->a_used_for_crank   = false;
    g->a_crank_accum      = 0.0f;
    memset(&g->last_move, 0, sizeof(Move));
    history_init(&g->history);
    history_push(&g->history, &g->board, NULL); /* initial state, no move */

    /* If AI plays first (rare), kick it off. */
    if (current_player_is_ai(g)) {
        g->mode = GAME_MODE_AI_THINKING;
        g->ai_pending = true;
    }
    game_refresh_highlights(g);
}

void game_to_title(GameState* g) {
    g->mode               = GAME_MODE_TITLE;
    g->review_offset      = 0;
    g->review_crank_accum = 0.0f;
    g->a_held             = false;
    g->a_used_for_crank   = false;
    g->a_crank_accum      = 0.0f;
    g->ai_pending         = false;
}

void game_move_cursor(GameState* g, int dx, int dy) {
    if (g->mode != GAME_MODE_FREE && g->mode != GAME_MODE_PIECE_SELECTED) return;
    g->cursor_x = clamp(g->cursor_x + dx, 0, BOARD_DIM - 1);
    g->cursor_y = clamp(g->cursor_y + dy, 0, BOARD_DIM - 1);
    /* Hover preview tracks the cursor in FREE mode. */
    if (g->mode == GAME_MODE_FREE) game_refresh_highlights(g);
}

void game_action_a(GameState* g) {
    if (g->mode == GAME_MODE_TITLE) {
        game_title_select(g);
        return;
    }
    if (g->mode == GAME_MODE_GAME_OVER) {
        game_to_title(g);
        return;
    }
    if (g->mode == GAME_MODE_AI_THINKING) return;

    /* If the player is reviewing the past, this A commits a branch — the
     * past state becomes live and future history is discarded. The same A
     * press then proceeds with normal selection against the new state. */
    if (g->review_offset != 0) {
        game_commit_review_branch(g);
    }

    Tile* under = &g->board.tiles[g->cursor_x][g->cursor_y];
    Piece* p    = &under->piece;

    if (g->mode == GAME_MODE_FREE) {
        if (p->type != PIECE_NONE && p->player == g->board.current_player) {
            select_piece(g, g->cursor_x, g->cursor_y);
        }
        return;
    }

    /* PIECE_SELECTED */
    if (g->cursor_x == g->selected_x && g->cursor_y == g->selected_y) {
        clear_selection(g);
        return;
    }
    if (p->type != PIECE_NONE && p->player == g->board.current_player) {
        clear_selection(g);
        select_piece(g, g->cursor_x, g->cursor_y);
        return;
    }
    Move chosen;
    if (game_is_valid_destination(g, g->cursor_x, g->cursor_y, &chosen)) {
        apply_move(&g->board, &chosen);
        clear_selection(g);
        after_move_applied(g, &chosen);
    }
}

void game_action_b(GameState* g) {
    if (g->mode == GAME_MODE_PIECE_SELECTED) {
        clear_selection(g);
    } else if (g->mode == GAME_MODE_GAME_OVER) {
        game_to_title(g);
    }
}

/* ---- Title menu ---- */

/* Title menu rows:
 *  0 — start PvP
 *  1 — start PvA
 *  2 — AI level
 *  3 — Map
 *  4 — Render */
#define TITLE_ROW_COUNT 5

void game_title_nav(GameState* g, int dy) {
    if (g->mode != GAME_MODE_TITLE) return;
    g->menu_row = (g->menu_row + dy + TITLE_ROW_COUNT) % TITLE_ROW_COUNT;
}

void game_title_change(GameState* g, int dx) {
    if (g->mode != GAME_MODE_TITLE) return;
    if (g->menu_row == 2) {
        int v = g->settings.ai_level + dx;
        if (v < AI_LEVEL_MIN) v = AI_LEVEL_MAX;
        if (v > AI_LEVEL_MAX) v = AI_LEVEL_MIN;
        g->settings.ai_level = v;
    } else if (g->menu_row == 3) {
        int v = ((int)g->settings.map + dx + (int)MAP_COUNT) % (int)MAP_COUNT;
        g->settings.map = (MapLayout)v;
    } else if (g->menu_row == 4) {
        g->settings.render = (g->settings.render == RENDER_FLAT) ? RENDER_ELEVATED : RENDER_FLAT;
    }
    /* nothing for rows 0/1 — those are start buttons */
}

void game_commit_review_branch(GameState* g) {
    if (g->review_offset == 0) return;
    int target_turn = g->board.turn_count + g->review_offset;
    const Board* snap = history_get(&g->history, target_turn);
    const Move*  prev = history_get_move(&g->history, target_turn);
    if (snap) {
        g->board = *snap;
        history_truncate_at(&g->history, target_turn);
    }
    g->review_offset      = 0;
    g->review_crank_accum = 0.0f;
    if (prev && prev->piece_type != PIECE_NONE) {
        g->last_move     = *prev;
        g->has_last_move = true;
    } else {
        g->has_last_move = false;
    }
    g->mode = (g->board.win_state != WIN_NONE) ? GAME_MODE_GAME_OVER : GAME_MODE_FREE;
    board_clear_highlights(&g->board);
    g->selected_x       = -1;
    g->selected_y       = -1;
    g->num_valid_moves  = 0;
    /* If after branching it's now the AI's turn, queue the hourglass for it. */
    if (g->board.win_state == WIN_NONE && current_player_is_ai(g)) {
        g->mode       = GAME_MODE_AI_THINKING;
        g->ai_pending = true;
    }
}

void game_title_select(GameState* g) {
    if (g->mode != GAME_MODE_TITLE) return;
    if (g->menu_row == 0) {
        g->settings.match = MATCH_PVP;
        game_reset(g);
    } else if (g->menu_row == 1) {
        g->settings.match     = MATCH_PVA;
        g->settings.ai_player = PLAYER_WHITE;
        game_reset(g);
    }
    /* rows 2 & 3 are settings: A does nothing */
}

/* ---- AI ---- */

bool game_ai_step(GameState* g) {
    if (!g->ai_pending) return true;

    /* First call for this turn: initialize search bookkeeping. */
    if (g->ai_next_depth == 0) {
        ai_level_config(g->settings.ai_level, &g->ai_target_depth, &g->ai_target_noise);
        g->ai_next_depth      = 1;
        g->ai_has_best        = false;
        g->ai_thinking_frames = 0;
    }

    /* Search one ply at the current depth. Noise applies only at the final
     * depth so that intermediate depths give clean move ordering. */
    int  depth = g->ai_next_depth;
    int  noise = (depth >= g->ai_target_depth) ? g->ai_target_noise : 0;
    Move m;
    if (ai_pick_move_at_depth(&g->board, depth, noise, &m)) {
        g->ai_best_move = m;
        g->ai_has_best  = true;
    }
    g->ai_next_depth++;

    if (g->ai_next_depth > g->ai_target_depth) {
        /* Done. Commit the chosen move (or accept that there is none). */
        if (g->ai_has_best) {
            apply_move(&g->board, &g->ai_best_move);
            after_move_applied(g, &g->ai_best_move);
        } else {
            g->mode = GAME_MODE_GAME_OVER;
        }
        g->ai_pending    = false;
        g->ai_next_depth = 0;
        g->ai_has_best   = false;
        return true;
    }
    return false;
}

/* ---- Visible board (live or replayed snapshot) ---- */

const Board* game_visible_board(const GameState* g) {
    if (g->mode == GAME_MODE_TITLE) return &g->board;
    if (g->review_offset != 0) {
        int target_turn = g->board.turn_count + g->review_offset;
        const Board* snap = history_get(&g->history, target_turn);
        if (snap) return snap;
    }
    return &g->board;
}
