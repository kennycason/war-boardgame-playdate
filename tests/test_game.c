#include <string.h>
#include "test.h"
#include "game.h"

static void test_game_init_default_state(void) {
    GameState g;
    game_init(&g);
    ASSERT_EQ(g.mode, GAME_MODE_FREE);
    ASSERT_EQ(g.board.current_player, PLAYER_BLACK);
    ASSERT_EQ(g.selected_x, -1);
    ASSERT_EQ(g.selected_y, -1);
    ASSERT_EQ(g.num_valid_moves, 0);
    ASSERT_FALSE(g.has_last_move);
    ASSERT_EQ(g.board.tiles[0][0].piece.type, PIECE_COMMANDER);
}

static void test_cursor_clamps_to_board(void) {
    GameState g;
    game_init(&g);
    g.cursor_x = 0; g.cursor_y = 0;
    game_move_cursor(&g, -5, -5);
    ASSERT_EQ(g.cursor_x, 0);
    ASSERT_EQ(g.cursor_y, 0);
    g.cursor_x = 10; g.cursor_y = 10;
    game_move_cursor(&g, 5, 5);
    ASSERT_EQ(g.cursor_x, 10);
    ASSERT_EQ(g.cursor_y, 10);
}

static void test_a_selects_own_piece(void) {
    GameState g;
    game_init(&g);
    /* Black infantry at (5,2) has 2 cardinal openings on turn 1. */
    g.cursor_x = 5; g.cursor_y = 2;
    game_action_a(&g);
    ASSERT_EQ(g.mode, GAME_MODE_PIECE_SELECTED);
    ASSERT_EQ(g.selected_x, 5);
    ASSERT_EQ(g.selected_y, 2);
    ASSERT_TRUE(g.num_valid_moves > 0);
}

static void test_a_ignores_enemy_piece(void) {
    GameState g;
    game_init(&g);
    g.cursor_x = 10; g.cursor_y = 10; /* white commander, black's turn */
    game_action_a(&g);
    ASSERT_EQ(g.mode, GAME_MODE_FREE);
    ASSERT_EQ(g.selected_x, -1);
}

static void test_b_cancels_selection(void) {
    GameState g;
    game_init(&g);
    g.cursor_x = 0; g.cursor_y = 0;
    game_action_a(&g);
    ASSERT_EQ(g.mode, GAME_MODE_PIECE_SELECTED);
    game_action_b(&g);
    ASSERT_EQ(g.mode, GAME_MODE_FREE);
    ASSERT_EQ(g.selected_x, -1);
}

static void test_a_on_self_deselects(void) {
    GameState g;
    game_init(&g);
    g.cursor_x = 0; g.cursor_y = 0;
    game_action_a(&g);
    ASSERT_EQ(g.mode, GAME_MODE_PIECE_SELECTED);
    /* Cursor still on the selected piece — A should deselect. */
    game_action_a(&g);
    ASSERT_EQ(g.mode, GAME_MODE_FREE);
}

static void test_a_on_other_own_piece_reselects(void) {
    GameState g;
    game_init(&g);
    g.cursor_x = 0; g.cursor_y = 0;
    game_action_a(&g);
    ASSERT_EQ(g.selected_x, 0);
    ASSERT_EQ(g.selected_y, 0);
    g.cursor_x = 3; g.cursor_y = 0; /* another black piece (tank) */
    game_action_a(&g);
    ASSERT_EQ(g.mode, GAME_MODE_PIECE_SELECTED);
    ASSERT_EQ(g.selected_x, 3);
    ASSERT_EQ(g.selected_y, 0);
}

static void test_a_on_valid_destination_executes(void) {
    GameState g;
    game_init(&g);
    /* Black tank at 3,0 has no easy move on day 1 (blocked by terrain/pieces).
     * Use the infantry at 5,2 which can move to 5,3. */
    g.cursor_x = 5; g.cursor_y = 2;
    game_action_a(&g);
    ASSERT_EQ(g.mode, GAME_MODE_PIECE_SELECTED);
    /* Move to an empty cardinal neighbor. */
    g.cursor_x = 5; g.cursor_y = 3;
    int prev_turn = g.board.turn_count;
    game_action_a(&g);
    ASSERT_EQ(g.mode, GAME_MODE_FREE);
    ASSERT_EQ(g.board.tiles[5][3].piece.type, PIECE_INFANTRY);
    ASSERT_EQ(g.board.turn_count, prev_turn + 1);
    ASSERT_EQ(g.board.current_player, PLAYER_WHITE);
    ASSERT_TRUE(g.has_last_move);
}

static void test_a_on_invalid_destination_does_nothing(void) {
    GameState g;
    game_init(&g);
    g.cursor_x = 5; g.cursor_y = 2;
    game_action_a(&g); /* select */
    g.cursor_x = 10; g.cursor_y = 10; /* far away */
    int prev_turn = g.board.turn_count;
    game_action_a(&g);
    /* selection should remain */
    ASSERT_EQ(g.mode, GAME_MODE_PIECE_SELECTED);
    ASSERT_EQ(g.board.turn_count, prev_turn);
}

static void test_highlights_are_set_for_valid_moves(void) {
    GameState g;
    game_init(&g);
    g.cursor_x = 5; g.cursor_y = 2;
    game_action_a(&g);
    ASSERT_EQ(g.board.tiles[5][2].highlight, HIGHLIGHT_SELECTED);
    ASSERT_EQ(g.board.tiles[5][3].highlight, HIGHLIGHT_MOVE);
}

void run_game_tests(void) {
    SECTION("Game state");
    RUN(test_game_init_default_state);
    RUN(test_cursor_clamps_to_board);
    RUN(test_a_selects_own_piece);
    RUN(test_a_ignores_enemy_piece);
    RUN(test_b_cancels_selection);
    RUN(test_a_on_self_deselects);
    RUN(test_a_on_other_own_piece_reselects);
    RUN(test_a_on_valid_destination_executes);
    RUN(test_a_on_invalid_destination_does_nothing);
    RUN(test_highlights_are_set_for_valid_moves);
}
