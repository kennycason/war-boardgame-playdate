#include <string.h>
#include "history.h"

void history_init(History* h) {
    memset(h, 0, sizeof(History));
    h->count = 0;
    h->head  = 0;
}

void history_push(History* h, const Board* b) {
    if (h->count < HISTORY_CAP) {
        h->snapshots[h->count] = *b;
        h->count++;
    } else {
        h->snapshots[h->head] = *b;
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

const Board* history_get(const History* h, int turn) {
    if (h->count == 0) return NULL;
    int oldest = history_oldest_turn(h);
    int newest = history_newest_turn(h);
    if (turn < oldest || turn > newest) return NULL;

    /* Snapshots are stored in turn order starting from h->head. */
    int offset_from_oldest = turn - oldest;
    int idx;
    if (h->count <= HISTORY_CAP) {
        idx = offset_from_oldest;
    } else {
        idx = (h->head + offset_from_oldest) % HISTORY_CAP;
    }
    return &h->snapshots[idx];
}
