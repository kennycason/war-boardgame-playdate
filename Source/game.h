#ifndef WAR_GAME_H
#define WAR_GAME_H

#include <stdbool.h>
#include "types.h"
#include "board.h"
#include "move.h"
#include "history.h"

typedef struct {
    Board    board;
    GameMode mode;
    Settings settings;
    History  history;

    /* cursor (board coords) */
    int  cursor_x, cursor_y;

    /* selection — valid only in PIECE_SELECTED mode */
    int  selected_x, selected_y;
    Move valid_moves[MAX_MOVES_PER_PIECE];
    int  num_valid_moves;

    /* last applied move — for sidebar */
    Move last_move;
    bool has_last_move;
    int  flash_counter;

    /* AI state */
    bool ai_pending;            /* deferred: hourglass this frame, AI runs next */
    int  ai_thinking_frames;    /* counts frames spent showing hourglass (for animation) */

    /* title menu state */
    int  menu_row;              /* 0=PvP, 1=PvA, 2=AI difficulty, 3=render mode */

    /* time-travel state */
    bool  b_held;
    int   review_offset;        /* 0 = current; negative = looking back N turns */
    float review_crank_accum;
} GameState;

void game_init(GameState* g);
void game_reset(GameState* g);
void game_to_title(GameState* g);

/* Cursor moves by (+/-1, 0) or (0, +/-1). Clamped to board. */
void game_move_cursor(GameState* g, int dx, int dy);

/* A button — context-sensitive confirm/select. */
void game_action_a(GameState* g);

/* B button — cancel selection / dismiss game-over. */
void game_action_b(GameState* g);

/* Title screen navigation. */
void game_title_nav(GameState* g, int dy);
void game_title_change(GameState* g, int dx);
void game_title_select(GameState* g);

/* Runs the AI for the current player. Returns true if a move was applied. */
bool game_run_ai(GameState* g);

/* Returns true if the (x,y) tile is in the current valid move set,
 * and writes the move into *out_move if non-null. */
bool game_is_valid_destination(const GameState* g, int x, int y, Move* out_move);

/* Refresh highlight overlays based on selection state. */
void game_refresh_highlights(GameState* g);

/* Returns the board to render this frame (live board, or replayed snapshot). */
const Board* game_visible_board(const GameState* g);

#endif
