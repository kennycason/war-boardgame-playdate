#include "test.h"
#include "board.h"
#include "move.h"
#include "ai.h"

static void test_ai_finds_capture_at_depth_1(void) {
    Board b;
    board_init(&b);
    board_place_piece(&b, PIECE_COMMANDER, PLAYER_BLACK, 0, 0);
    board_place_piece(&b, PIECE_COMMANDER, PLAYER_WHITE, 10, 10);
    /* Free black tank, free white pawn one tile away — black should take it. */
    board_place_piece(&b, PIECE_TANK,     PLAYER_BLACK, 5, 5);
    board_place_piece(&b, PIECE_INFANTRY, PLAYER_WHITE, 6, 5);
    /* Give both sides at least one attacker so win-check doesn't fire. */
    board_place_piece(&b, PIECE_INFANTRY, PLAYER_BLACK, 4, 0);
    board_place_piece(&b, PIECE_INFANTRY, PLAYER_WHITE, 9, 10);

    Move m;
    ASSERT_TRUE(ai_pick_move(&b, AI_EASY, &m));
    ASSERT_EQ(m.from_x, 5);
    ASSERT_EQ(m.from_y, 5);
    ASSERT_EQ(m.to_x, 6);
    ASSERT_EQ(m.to_y, 5);
    ASSERT_EQ(m.move_type, MOVE_TYPE_ATTACK);
}

static void test_ai_takes_higher_value_target(void) {
    Board b;
    board_init(&b);
    board_place_piece(&b, PIECE_COMMANDER, PLAYER_BLACK, 0, 0);
    board_place_piece(&b, PIECE_COMMANDER, PLAYER_WHITE, 10, 10);
    /* Black tank can attack two adjacent enemies: infantry (1) and bomber (5).
     * AI should grab the bomber. */
    board_place_piece(&b, PIECE_TANK,     PLAYER_BLACK, 5, 5);
    board_place_piece(&b, PIECE_INFANTRY, PLAYER_WHITE, 4, 5);
    board_place_piece(&b, PIECE_BOMBER,   PLAYER_WHITE, 6, 5);
    board_place_piece(&b, PIECE_INFANTRY, PLAYER_BLACK, 0, 1);

    Move m;
    ASSERT_TRUE(ai_pick_move(&b, AI_NORMAL, &m));
    ASSERT_EQ(m.move_type, MOVE_TYPE_ATTACK);
    ASSERT_EQ(m.to_x, 6);
    ASSERT_EQ(m.to_y, 5);
    ASSERT_EQ(m.destroyed_type, PIECE_BOMBER);
}

static void test_ai_grabs_commander_at_depth_1(void) {
    Board b;
    board_init(&b);
    board_place_piece(&b, PIECE_COMMANDER, PLAYER_BLACK, 0, 0);
    /* White commander exposed, black tank can reach it. */
    board_place_piece(&b, PIECE_COMMANDER, PLAYER_WHITE, 6, 5);
    board_place_piece(&b, PIECE_TANK,      PLAYER_BLACK, 5, 5);
    board_place_piece(&b, PIECE_INFANTRY,  PLAYER_BLACK, 0, 1);
    board_place_piece(&b, PIECE_INFANTRY,  PLAYER_WHITE, 9, 10);

    Move m;
    ASSERT_TRUE(ai_pick_move(&b, AI_EASY, &m));
    ASSERT_EQ(m.move_type, MOVE_TYPE_ATTACK);
    ASSERT_EQ(m.destroyed_type, PIECE_COMMANDER);
}

static void test_evaluate_symmetric(void) {
    /* On a clean fresh board, eval should be roughly 0 from either side. */
    Board b;
    board_init(&b);
    board_setup_pieces(&b);
    int black_eval = ai_evaluate(&b, PLAYER_BLACK);
    int white_eval = ai_evaluate(&b, PLAYER_WHITE);
    /* Allow some slack due to position bonuses computed differently. */
    int diff = black_eval - white_eval;
    if (diff < 0) diff = -diff;
    ASSERT_TRUE(diff < 50);
}

static void test_evaluate_material_advantage(void) {
    Board b;
    board_init(&b);
    board_place_piece(&b, PIECE_COMMANDER, PLAYER_BLACK, 0, 0);
    board_place_piece(&b, PIECE_COMMANDER, PLAYER_WHITE, 10, 10);
    /* Black has tank + bomber, white has just infantry. */
    board_place_piece(&b, PIECE_TANK,     PLAYER_BLACK, 3, 3);
    board_place_piece(&b, PIECE_BOMBER,   PLAYER_BLACK, 4, 4);
    board_place_piece(&b, PIECE_INFANTRY, PLAYER_WHITE, 7, 7);
    int e = ai_evaluate(&b, PLAYER_BLACK);
    ASSERT_TRUE(e > 0);
    int ew = ai_evaluate(&b, PLAYER_WHITE);
    ASSERT_TRUE(ew < 0);
}

void run_ai_tests(void) {
    SECTION("AI");
    RUN(test_evaluate_symmetric);
    RUN(test_evaluate_material_advantage);
    RUN(test_ai_finds_capture_at_depth_1);
    RUN(test_ai_takes_higher_value_target);
    RUN(test_ai_grabs_commander_at_depth_1);
}
