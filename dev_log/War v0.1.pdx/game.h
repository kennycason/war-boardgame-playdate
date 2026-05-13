#ifndef WAR_GAME_H
#define WAR_GAME_H

#include <stdbool.h>
#include "types.h"
#include "board.h"
#include "move.h"

typedef struct {
    Board    board;
    GameMode mode;

    /* cursor (board coords) */
    int  cursor_x, cursor_y;

    /* selection state — valid only in PIECE_SELECTED mode */
    int  selected_x, selected_y;
    Move valid_moves[MAX_MOVES_PER_PIECE];
    int  num_valid_moves;

    /* last applied move — for sidebar history */
    Move last_move;
    bool has_last_move;

    /* animation/transient — used by renderer */
    int  flash_counter; /* counts down frames for last-move flash */
} GameState;

void game_init(GameState* g);
void game_reset(GameState* g);

/* Cursor moves by (+/-1, 0) or (0, +/-1). Clamped to board. */
void game_move_cursor(GameState* g, int dx, int dy);

/* A button — context-sensitive confirm/select. */
void game_action_a(GameState* g);

/* B button — cancel selection / dismiss game-over. */
void game_action_b(GameState* g);

/* Returns true if the (x,y) tile is in the current valid move set,
 * and writes the move into *out_move if non-null. */
bool game_is_valid_destination(const GameState* g, int x, int y, Move* out_move);

/* Refresh highlight overlays based on selection state. */
void game_refresh_highlights(GameState* g);

#endif
