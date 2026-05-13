#ifndef WAR_HISTORY_H
#define WAR_HISTORY_H

#include <stdbool.h>
#include "types.h"
#include "board.h"
#include "move.h"

/* Ring buffer of recent board snapshots + the move that produced each.
 * Index 0 is the initial state (its move slot is unused). If count exceeds
 * HISTORY_CAP, older entries are dropped — review can only go back
 * HISTORY_CAP turns. */
typedef struct {
    Board snapshots[HISTORY_CAP];
    Move  moves[HISTORY_CAP];     /* move that led TO snapshots[i] (undef when i corresponds to turn 0) */
    int   count;                  /* total entries pushed (may exceed HISTORY_CAP) */
    int   head;                   /* ring-buffer head index */
} History;

void           history_init(History* h);

/* If `m` is NULL, an empty move record is stored (used for the initial state). */
void           history_push(History* h, const Board* b, const Move* m);

/* `turn` is in the same range as Board.turn_count. Returns NULL if outside
 * the retained window. */
const Board*   history_get(const History* h, int turn);
const Move*    history_get_move(const History* h, int turn);

/* Oldest and newest retained turn indices. */
int            history_oldest_turn(const History* h);
int            history_newest_turn(const History* h);

/* Drop every snapshot whose turn_count > `turn`. The newest retained entry
 * after this call has turn == `turn`. Used when the player branches by
 * making a move from a past state. */
void           history_truncate_at(History* h, int turn);

#endif
