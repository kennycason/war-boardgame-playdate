#include <string.h>
#include "history.h"

void history_init(History* h) {
    memset(h, 0, sizeof(History));
    h->count = 0;
    h->head  = 0;
}

void history_push(History* h, const Board* b, const Move* m) {
    Move m_local;
    if (m) m_local = *m;
    else   memset(&m_local, 0, sizeof(Move));

    if (h->count < HISTORY_CAP) {
        h->snapshots[h->count] = *b;
        h->moves[h->count]     = m_local;
        h->count++;
    } else {
        h->snapshots[h->head] = *b;
        h->moves[h->head]     = m_local;
        h->head = (h->head + 1) % HISTORY_CAP;
        h->count++;
    }
}

int history_oldest_turn(const History* h) {
    if (h->count == 0) return 0;
    int newest = history_newest_turn(h);
    int oldest = newest - (h->count < HISTORY_CAP ? h->count - 1 : HISTORY_CAP - 1);
    if (oldest < 0) oldest = 0;
    return oldest;
}

int history_newest_turn(const History* h) {
    if (h->count == 0) return 0;
    /* The newest snapshot's turn equals the Board.turn_count stored in it. */
    int newest_index;
    if (h->count <= HISTORY_CAP) newest_index = h->count - 1;
    else newest_index = (h->head + HISTORY_CAP - 1) % HISTORY_CAP;
    return h->snapshots[newest_index].turn_count;
}

static int history_index_for_turn(const History* h, int turn) {
    if (h->count == 0) return -1;
    int oldest = history_oldest_turn(h);
    int newest = history_newest_turn(h);
    if (turn < oldest || turn > newest) return -1;
    int offset = turn - oldest;
    if (h->count <= HISTORY_CAP) return offset;
    return (h->head + offset) % HISTORY_CAP;
}

const Board* history_get(const History* h, int turn) {
    int idx = history_index_for_turn(h, turn);
    if (idx < 0) return NULL;
    return &h->snapshots[idx];
}

const Move* history_get_move(const History* h, int turn) {
    int idx = history_index_for_turn(h, turn);
    if (idx < 0) return NULL;
    return &h->moves[idx];
}
