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

    /* AI state — iterative deepening, one ply per frame. The board freezes
     * for the duration of a single depth's αβ search but the UI gets to
     * redraw between depths. */
    bool ai_pending;            /* an AI move is required */
    int  ai_thinking_frames;    /* frames spent showing the hourglass (drives the dot animation) */
    int  ai_target_depth;       /* depth we want to reach for this move */
    int  ai_target_noise;       /* noise % at the final depth */
    int  ai_next_depth;         /* next depth to search (1..target) */
    Move ai_best_move;          /* best move found so far */
    bool ai_has_best;           /* whether ai_best_move is valid */

    /* title menu state */
    int  menu_row;              /* 0=PvP, 1=PvA, 2=AI difficulty, 3=render mode */

    /* time-travel state — plain crank scrubs through history. */
    int   review_offset;        /* 0 = current; negative = looking back N turns */
    float review_crank_accum;

    /* A-held + crank changes ai_level; we defer the A "click" action until
     * release so a hold-with-crank does not also fire a normal A press. */
    bool  a_held;
    bool  a_used_for_crank;
    float a_crank_accum;
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

/* Performs one iteration-deepening step (one ply). Returns true when the
 * full search at the target depth has completed AND the move has been
 * applied. Returns false if more work remains across frames. */
bool game_ai_step(GameState* g);

/* Commits the currently-reviewed past state as the new live state, truncating
 * future history. No-op if review_offset == 0. */
void game_commit_review_branch(GameState* g);

/* Returns true if the (x,y) tile is in the current valid move set,
 * and writes the move into *out_move if non-null. */
bool game_is_valid_destination(const GameState* g, int x, int y, Move* out_move);

/* Refresh highlight overlays based on selection state. */
void game_refresh_highlights(GameState* g);

/* Returns the board to render this frame (live board, or replayed snapshot). */
const Board* game_visible_board(const GameState* g);

#endif
