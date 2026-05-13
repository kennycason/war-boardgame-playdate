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
    /* Level 3 = depth 1, no randomness — should always grab the free capture. */
    ASSERT_TRUE(ai_pick_move(&b, 3, &m));
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
    /* Level 6 = depth 2, no randomness. */
    ASSERT_TRUE(ai_pick_move(&b, 6, &m));
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
    ASSERT_TRUE(ai_pick_move(&b, 3, &m));
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

static void test_ai_level_config_within_bounds(void) {
    int d, n;
    for (int lvl = AI_LEVEL_MIN; lvl <= AI_LEVEL_MAX; lvl++) {
        ai_level_config(lvl, &d, &n);
        ASSERT_TRUE(d >= 1 && d <= 3);
        ASSERT_TRUE(n >= 0 && n <= 100);
    }
    /* Out-of-range levels are clamped, not crashy. */
    ai_level_config(-5, &d, &n);
    ASSERT_TRUE(d >= 1);
    ai_level_config(99, &d, &n);
    ASSERT_TRUE(d <= 3);
}

static void test_ai_level_progression_monotonic(void) {
    int d1, n1, d3, n3, d6, n6, d9, n9;
    ai_level_config(1, &d1, &n1);
    ai_level_config(3, &d3, &n3);
    ai_level_config(6, &d6, &n6);
    ai_level_config(9, &d9, &n9);
    ASSERT_TRUE(d9 >= d6);
    ASSERT_TRUE(d6 >= d3);
    ASSERT_TRUE(d3 >= d1);
    /* Highest level — no noise (strongest play). */
    ASSERT_EQ(n9, 0);
    /* Within-depth-tier: cleanest play is the "no noise" tier (3, 6, 9). */
    ASSERT_EQ(n3, 0);
    ASSERT_EQ(n6, 0);
}

static void test_ai_pick_move_at_depth_finds_capture(void) {
    Board b;
    board_init(&b);
    board_place_piece(&b, PIECE_COMMANDER, PLAYER_BLACK, 0, 0);
    board_place_piece(&b, PIECE_COMMANDER, PLAYER_WHITE, 10, 10);
    board_place_piece(&b, PIECE_TANK,      PLAYER_BLACK, 5, 5);
    board_place_piece(&b, PIECE_INFANTRY,  PLAYER_WHITE, 6, 5);
    board_place_piece(&b, PIECE_INFANTRY,  PLAYER_BLACK, 4, 0);
    board_place_piece(&b, PIECE_INFANTRY,  PLAYER_WHITE, 9, 10);
    Move m;
    ASSERT_TRUE(ai_pick_move_at_depth(&b, 1, 0, &m));
    ASSERT_EQ(m.move_type, MOVE_TYPE_ATTACK);
    ASSERT_EQ(m.to_x, 6);
    ASSERT_EQ(m.to_y, 5);
}

static void test_ai_actually_searches(void) {
    /* Sanity check: depth 2 explores more nodes than depth 1. Verifies
     * the αβ search is doing real work for higher levels. */
    Board b;
    board_init(&b);
    board_setup_pieces(&b);
    Move m;
    ai_pick_move_at_depth(&b, 1, 0, &m);
    int n1 = ai_last_node_count();
    ai_pick_move_at_depth(&b, 2, 0, &m);
    int n2 = ai_last_node_count();
    ASSERT_TRUE(n1 > 0);
    ASSERT_TRUE(n2 > n1);
}

void run_ai_tests(void) {
    SECTION("AI");
    RUN(test_evaluate_symmetric);
    RUN(test_evaluate_material_advantage);
    RUN(test_ai_finds_capture_at_depth_1);
    RUN(test_ai_takes_higher_value_target);
    RUN(test_ai_grabs_commander_at_depth_1);
    RUN(test_ai_level_config_within_bounds);
    RUN(test_ai_level_progression_monotonic);
    RUN(test_ai_pick_move_at_depth_finds_capture);
    RUN(test_ai_actually_searches);
}
