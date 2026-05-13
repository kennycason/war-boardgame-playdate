#include <string.h>
#include "game.h"

static int clamp(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static void clear_selection(GameState* g) {
    g->selected_x      = -1;
    g->selected_y      = -1;
    g->num_valid_moves = 0;
    g->mode            = (g->board.win_state == WIN_NONE) ? GAME_MODE_FREE : GAME_MODE_GAME_OVER;
    board_clear_highlights(&g->board);
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
        /* Don't overwrite SELECTED. */
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

void game_init(GameState* g) {
    memset(g, 0, sizeof(GameState));
    game_reset(g);
}

void game_reset(GameState* g) {
    board_reset(&g->board);
    g->mode            = GAME_MODE_FREE;
    g->cursor_x        = 3;
    g->cursor_y        = 2;
    g->selected_x      = -1;
    g->selected_y      = -1;
    g->num_valid_moves = 0;
    g->has_last_move   = false;
    g->flash_counter   = 0;
    memset(&g->last_move, 0, sizeof(Move));
}

void game_move_cursor(GameState* g, int dx, int dy) {
    if (g->mode == GAME_MODE_GAME_OVER || g->mode == GAME_MODE_TITLE) return;
    g->cursor_x = clamp(g->cursor_x + dx, 0, BOARD_DIM - 1);
    g->cursor_y = clamp(g->cursor_y + dy, 0, BOARD_DIM - 1);
}

void game_action_a(GameState* g) {
    if (g->mode == GAME_MODE_TITLE) {
        game_reset(g);
        return;
    }
    if (g->mode == GAME_MODE_GAME_OVER) {
        game_reset(g);
        return;
    }

    Tile* under = &g->board.tiles[g->cursor_x][g->cursor_y];
    Piece* p    = &under->piece;

    if (g->mode == GAME_MODE_FREE) {
        /* Try to select a piece of the current player. */
        if (p->type != PIECE_NONE && p->player == g->board.current_player) {
            select_piece(g, g->cursor_x, g->cursor_y);
        }
        return;
    }

    /* PIECE_SELECTED */
    /* Cursor on selected piece → deselect. */
    if (g->cursor_x == g->selected_x && g->cursor_y == g->selected_y) {
        clear_selection(g);
        return;
    }
    /* Cursor on another own piece → re-select. */
    if (p->type != PIECE_NONE && p->player == g->board.current_player) {
        clear_selection(g);
        select_piece(g, g->cursor_x, g->cursor_y);
        return;
    }
    /* Cursor on a valid destination → apply. */
    Move chosen;
    if (game_is_valid_destination(g, g->cursor_x, g->cursor_y, &chosen)) {
        apply_move(&g->board, &chosen);
        g->last_move     = chosen;
        g->has_last_move = true;
        g->flash_counter = 12; /* ~0.4s @30fps */
        clear_selection(g);
        if (g->board.win_state != WIN_NONE) {
            g->mode = GAME_MODE_GAME_OVER;
        }
    }
}

void game_action_b(GameState* g) {
    if (g->mode == GAME_MODE_PIECE_SELECTED) {
        clear_selection(g);
    } else if (g->mode == GAME_MODE_GAME_OVER) {
        game_reset(g);
    }
}
