#include "test.h"
#include "history.h"
#include "board.h"
#include "move.h"

static void test_history_empty(void) {
    History h;
    history_init(&h);
    ASSERT_EQ(h.count, 0);
    ASSERT_TRUE(history_get(&h, 0) == NULL);
}

static void test_history_push_and_get(void) {
    History h;
    history_init(&h);

    Board b;
    board_init(&b);
    history_push(&h, &b, NULL);          /* turn 0 */
    b.turn_count = 1;
    b.black_score = 5;
    history_push(&h, &b, NULL);          /* turn 1 */
    b.turn_count = 2;
    b.black_score = 12;
    history_push(&h, &b, NULL);          /* turn 2 */

    ASSERT_EQ(h.count, 3);
    ASSERT_EQ(history_oldest_turn(&h), 0);
    ASSERT_EQ(history_newest_turn(&h), 2);

    const Board* s0 = history_get(&h, 0);
    const Board* s1 = history_get(&h, 1);
    const Board* s2 = history_get(&h, 2);
    ASSERT_TRUE(s0 != NULL);
    ASSERT_TRUE(s1 != NULL);
    ASSERT_TRUE(s2 != NULL);
    ASSERT_EQ(s0->turn_count,  0);
    ASSERT_EQ(s0->black_score, 0);
    ASSERT_EQ(s1->black_score, 5);
    ASSERT_EQ(s2->black_score, 12);
}

static void test_history_overflow_drops_oldest(void) {
    History h;
    history_init(&h);
    Board b;
    board_init(&b);

    /* Push HISTORY_CAP + 10 snapshots, increasing turn_count. */
    for (int i = 0; i < HISTORY_CAP + 10; i++) {
        b.turn_count  = i;
        b.black_score = i * 2;
        history_push(&h, &b, NULL);
    }

    ASSERT_EQ(h.count, HISTORY_CAP + 10);
    ASSERT_EQ(history_newest_turn(&h), HISTORY_CAP + 9);
    ASSERT_EQ(history_oldest_turn(&h), 10);

    /* Oldest retained snapshot should still have the right data. */
    const Board* oldest = history_get(&h, 10);
    ASSERT_TRUE(oldest != NULL);
    ASSERT_EQ(oldest->black_score, 20);

    /* Newest snapshot data. */
    const Board* newest = history_get(&h, HISTORY_CAP + 9);
    ASSERT_TRUE(newest != NULL);
    ASSERT_EQ(newest->black_score, (HISTORY_CAP + 9) * 2);

    /* Out-of-range queries return NULL. */
    ASSERT_TRUE(history_get(&h, 9) == NULL);
    ASSERT_TRUE(history_get(&h, HISTORY_CAP + 10) == NULL);
}

static void test_history_replay_after_move(void) {
    /* Verify that a snapshot captures piece state correctly. */
    History h;
    history_init(&h);
    Board b;
    board_init(&b);
    board_place_piece(&b, PIECE_TANK,     PLAYER_BLACK, 5, 5);
    board_place_piece(&b, PIECE_INFANTRY, PLAYER_WHITE, 6, 5);
    history_push(&h, &b, NULL);

    Move m;
    ASSERT_TRUE(find_move(&b, 5, 5, 6, 5, 0, &m));
    apply_move(&b, &m);
    history_push(&h, &b, NULL);

    /* Snapshot 0 should still show the original setup. */
    const Board* s0 = history_get(&h, 0);
    ASSERT_TRUE(s0 != NULL);
    ASSERT_EQ(s0->tiles[5][5].piece.type, PIECE_TANK);
    ASSERT_EQ(s0->tiles[6][5].piece.type, PIECE_INFANTRY);

    /* Snapshot 1 (after capture) should show the moved tank. */
    const Board* s1 = history_get(&h, 1);
    ASSERT_TRUE(s1 != NULL);
    ASSERT_EQ(s1->tiles[5][5].piece.type, PIECE_NONE);
    ASSERT_EQ(s1->tiles[6][5].piece.type, PIECE_TANK);
}

void run_history_tests(void) {
    SECTION("History");
    RUN(test_history_empty);
    RUN(test_history_push_and_get);
    RUN(test_history_overflow_drops_oldest);
    RUN(test_history_replay_after_move);
}
