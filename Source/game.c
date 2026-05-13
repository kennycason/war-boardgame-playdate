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
    board_clear_highlights(&g->board);
    if (g->board.win_state != WIN_NONE) g->mode = GAME_MODE_GAME_OVER;
    else if (g->mode == GAME_MODE_PIECE_SELECTED) g->mode = GAME_MODE_FREE;
}

static void select_piece(GameState* g, int x, int y) {
    g->selected_x      = x;
    g->selected_y      = y;
    g->num_valid_moves = generate_moves(&g->board, x, y, g->valid_moves, MAX_MOVES_PER_PIECE);
    g->mode            = GAME_MODE_PIECE_SELECTED;
    game_refresh_highlights(g);
}

void game_refresh_highlights(GameState* g) {
    board_clear_highlights(&g->board);
    if (g->mode != GAME_MODE_PIECE_SELECTED) return;
    g->board.tiles[g->selected_x][g->selected_y].highlight = HIGHLIGHT_SELECTED;
    for (int i = 0; i < g->num_valid_moves; i++) {
        const Move* m = &g->valid_moves[i];
        TileHighlight h = (m->move_type == MOVE_TYPE_ATTACK) ? HIGHLIGHT_ATTACK : HIGHLIGHT_MOVE;
        if (g->board.tiles[m->to_x][m->to_y].highlight != HIGHLIGHT_SELECTED) {
            g->board.tiles[m->to_x][m->to_y].highlight = h;
        }
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
    history_push(&g->history, &g->board);

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
    g->settings.match         = MATCH_PVP;
    g->settings.ai_difficulty = AI_NORMAL;
    g->settings.ai_player     = PLAYER_WHITE;
    g->settings.render        = RENDER_ELEVATED;
    g->mode                   = GAME_MODE_TITLE;
    g->menu_row               = 0;
    g->selected_x             = -1;
    g->selected_y             = -1;
}

void game_reset(GameState* g) {
    board_reset(&g->board);
    g->mode               = GAME_MODE_FREE;
    g->cursor_x           = 3;
    g->cursor_y           = 2;
    g->selected_x         = -1;
    g->selected_y         = -1;
    g->num_valid_moves    = 0;
    g->has_last_move      = false;
    g->flash_counter      = 0;
    g->ai_pending         = false;
    g->ai_thinking_frames = 0;
    g->review_offset      = 0;
    g->review_crank_accum = 0.0f;
    g->b_held             = false;
    memset(&g->last_move, 0, sizeof(Move));
    history_init(&g->history);
    history_push(&g->history, &g->board);

    /* If AI plays first (rare), kick it off. */
    if (current_player_is_ai(g)) {
        g->mode = GAME_MODE_AI_THINKING;
        g->ai_pending = true;
    }
}

void game_to_title(GameState* g) {
    g->mode               = GAME_MODE_TITLE;
    g->review_offset      = 0;
    g->review_crank_accum = 0.0f;
    g->b_held             = false;
    g->ai_pending         = false;
}

void game_move_cursor(GameState* g, int dx, int dy) {
    if (g->mode != GAME_MODE_FREE && g->mode != GAME_MODE_PIECE_SELECTED) return;
    g->cursor_x = clamp(g->cursor_x + dx, 0, BOARD_DIM - 1);
    g->cursor_y = clamp(g->cursor_y + dy, 0, BOARD_DIM - 1);
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

void game_title_nav(GameState* g, int dy) {
    if (g->mode != GAME_MODE_TITLE) return;
    g->menu_row = (g->menu_row + dy + 4) % 4;
}

void game_title_change(GameState* g, int dx) {
    if (g->mode != GAME_MODE_TITLE) return;
    if (g->menu_row == 2) {
        int v = (int)g->settings.ai_difficulty + dx;
        if (v < 0) v = 2;
        if (v > 2) v = 0;
        g->settings.ai_difficulty = (AIDifficulty)v;
    } else if (g->menu_row == 3) {
        g->settings.render = (g->settings.render == RENDER_FLAT) ? RENDER_ELEVATED : RENDER_FLAT;
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

bool game_run_ai(GameState* g) {
    if (!g->ai_pending) return false;
    Move m;
    bool ok = ai_pick_move(&g->board, g->settings.ai_difficulty, &m);
    g->ai_pending = false;
    if (!ok) {
        /* No moves — current player loses by stalemate */
        g->mode = GAME_MODE_GAME_OVER;
        return false;
    }
    apply_move(&g->board, &m);
    after_move_applied(g, &m);
    return true;
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
