#ifndef WAR_MOVE_H
#define WAR_MOVE_H

#include <stdbool.h>
#include "types.h"
#include "board.h"

typedef struct {
    int       from_x, from_y;
    int       to_x,   to_y;
    MoveType  move_type;
    PieceType piece_type;
    Player    player;
    int       score;
    int       elevation_delta; /* +1 / -1 for excavator */
    PieceType destroyed_type;
    Player    destroyed_player;
    /* outcome flags populated by apply_move */
    bool      intercepted;
    int       intercept_x, intercept_y;
} Move;

/* Generate every legal move for the piece at (x,y).
 * Returns number of moves written (≤ max_moves). */
int  generate_moves(Board* b, int x, int y, Move* out, int max_moves);

/* Apply a move and switch turn. Updates board state, scores, win state. */
void apply_move(Board* b, Move* mv);

/* Find a generated move with matching from/to (and elev_delta for excavator).
 * Returns true and copies into *out. */
bool find_move(Board* b, int from_x, int from_y, int to_x, int to_y,
               int elevation_delta, Move* out);

#endif
