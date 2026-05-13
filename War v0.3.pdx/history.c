#include <string.h>
#include "history.h"

/* Internal: head + count + ring. count is "currently retained" (0..HISTORY_CAP).
 * Turn numbers are read from the stored Board.turn_count fields, so the ring
 * doesn't need a separate turn-tracker. */

void history_init(History* h) {
    memset(h, 0, sizeof(History));
    h->count = 0;
    h->head  = 0;
}

void history_push(History* h, const Board* b, const Move* m) {
    Move m_local;
    if (m) m_local = *m;
    else   memset(&m_local, 0, sizeof(Move));

    int idx;
    if (h->count < HISTORY_CAP) {
        idx = (h->head + h->count) % HISTORY_CAP;
        h->count++;
    } else {
        idx = h->head;
        h->head = (h->head + 1) % HISTORY_CAP;
        /* count stays at HISTORY_CAP */
    }
    h->snapshots[idx] = *b;
    h->moves[idx]     = m_local;
}

int history_oldest_turn(const History* h) {
    if (h->count == 0) return 0;
    return h->snapshots[h->head].turn_count;
}

int history_newest_turn(const History* h) {
    if (h->count == 0) return 0;
    int tail = (h->head + h->count - 1) % HISTORY_CAP;
    return h->snapshots[tail].turn_count;
}

static int index_for_turn(const History* h, int turn) {
    if (h->count == 0) return -1;
    int oldest = history_oldest_turn(h);
    int offset = turn - oldest;
    if (offset < 0 || offset >= h->count) return -1;
    return (h->head + offset) % HISTORY_CAP;
}

const Board* history_get(const History* h, int turn) {
    int idx = index_for_turn(h, turn);
    return (idx < 0) ? NULL : &h->snapshots[idx];
}

const Move* history_get_move(const History* h, int turn) {
    int idx = index_for_turn(h, turn);
    return (idx < 0) ? NULL : &h->moves[idx];
}

void history_truncate_at(History* h, int turn) {
    if (h->count == 0) return;
    int newest = history_newest_turn(h);
    if (turn >= newest) return;
    int oldest = history_oldest_turn(h);
    if (turn < oldest) {
        h->count = 0;
        h->head  = 0;
        return;
    }
    h->count = turn - oldest + 1;
    /* head doesn't move — we're shrinking the tail. */
}
