#ifndef WAR_HISTORY_H
#define WAR_HISTORY_H

#include <stdbool.h>
#include "types.h"
#include "board.h"

/* Ring buffer of recent board snapshots. Index 0 is the initial state;
 * index `count - 1` is the most recent. If count > HISTORY_CAP, older
 * entries are silently dropped — review can only go back HISTORY_CAP turns. */
typedef struct {
    Board snapshots[HISTORY_CAP];
    int   count;       /* total snapshots pushed (may exceed HISTORY_CAP) */
    int   head;        /* index in `snapshots[]` of the oldest retained snapshot */
} History;

void           history_init(History* h);
void           history_push(History* h, const Board* b);

/* `turn` is in the same range as Board.turn_count. Returns NULL if outside
 * the retained window. */
const Board*   history_get(const History* h, int turn);

/* Oldest and newest retained turn indices. */
int            history_oldest_turn(const History* h);
int            history_newest_turn(const History* h);

#endif
