#include <string.h>
#include "test.h"
#include "game.h"

static void test_game_init_default_state(void) {
    GameState g;
    game_init(&g);
    game_reset(&g);
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
    game_reset(&g);
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
    game_reset(&g);
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
    game_reset(&g);
    g.cursor_x = 10; g.cursor_y = 10; /* white commander, black's turn */
    game_action_a(&g);
    ASSERT_EQ(g.mode, GAME_MODE_FREE);
    ASSERT_EQ(g.selected_x, -1);
}

static void test_b_cancels_selection(void) {
    GameState g;
    game_init(&g);
    game_reset(&g);
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
    game_reset(&g);
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
    game_reset(&g);
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
    game_reset(&g);
    /* Black infantry at (5,2) can step east to (6,2) — both reachable on the
     * default terrain (elevation 1 → 0, diff 1). */
    g.cursor_x = 5; g.cursor_y = 2;
    game_action_a(&g);
    ASSERT_EQ(g.mode, GAME_MODE_PIECE_SELECTED);
    g.cursor_x = 6; g.cursor_y = 2;
    int prev_turn = g.board.turn_count;
    game_action_a(&g);
    ASSERT_EQ(g.mode, GAME_MODE_FREE);
    ASSERT_EQ(g.board.tiles[6][2].piece.type, PIECE_INFANTRY);
    ASSERT_EQ(g.board.turn_count, prev_turn + 1);
    ASSERT_EQ(g.board.current_player, PLAYER_WHITE);
    ASSERT_TRUE(g.has_last_move);
}

static void test_a_on_invalid_destination_does_nothing(void) {
    GameState g;
    game_init(&g);
    game_reset(&g);
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
    game_reset(&g);
    g.cursor_x = 5; g.cursor_y = 2;
    game_action_a(&g);
    ASSERT_EQ(g.board.tiles[5][2].highlight, HIGHLIGHT_SELECTED);
    ASSERT_EQ(g.board.tiles[6][2].highlight, HIGHLIGHT_MOVE);
}

/* Helper: make one full black-then-white turn pair on an open setup. */
static void play_two_moves(GameState* g) {
    /* Black infantry (5,2) → (6,2) */
    g->cursor_x = 5; g->cursor_y = 2;
    game_action_a(g);
    g->cursor_x = 6; g->cursor_y = 2;
    game_action_a(g);

    /* White infantry (5,8) → (4,8) */
    g->cursor_x = 5; g->cursor_y = 8;
    game_action_a(g);
    g->cursor_x = 4; g->cursor_y = 8;
    game_action_a(g);
}

static void test_commit_review_branch_truncates_future(void) {
    GameState g;
    game_init(&g);
    g.settings.match = MATCH_PVP;
    game_reset(&g);
    play_two_moves(&g);
    ASSERT_EQ(g.board.turn_count, 2);
    ASSERT_EQ(history_newest_turn(&g.history), 2);

    /* Scrub back 1 turn and commit. */
    g.review_offset = -1;
    game_commit_review_branch(&g);

    ASSERT_EQ(g.review_offset, 0);
    ASSERT_EQ(g.board.turn_count, 1);
    ASSERT_EQ(history_newest_turn(&g.history), 1);
    ASSERT_TRUE(history_get(&g.history, 2) == NULL);
    /* Turn 1 happened after black moved, so it's white's turn now. */
    ASSERT_EQ(g.board.current_player, PLAYER_WHITE);
    /* The piece that white moved should still be at its original spot. */
    ASSERT_EQ(g.board.tiles[5][8].piece.type, PIECE_INFANTRY);
    ASSERT_EQ(g.board.tiles[5][8].piece.player, PLAYER_WHITE);
}

static void test_commit_review_branch_noop_at_zero(void) {
    GameState g;
    game_init(&g);
    g.settings.match = MATCH_PVP;
    game_reset(&g);
    play_two_moves(&g);
    int turn_before = g.board.turn_count;
    int hist_before = history_newest_turn(&g.history);

    game_commit_review_branch(&g);

    ASSERT_EQ(g.board.turn_count, turn_before);
    ASSERT_EQ(history_newest_turn(&g.history), hist_before);
}

/* Regression: scrubbing back to a snapshot where it's the AI's turn and then
 * pressing A used to fall through into normal piece-selection logic. If the
 * cursor happened to be over an AI piece, the player accidentally "selected"
 * it (mode → PIECE_SELECTED) and the queued AI step never fired. */
static void test_commit_review_to_ai_turn_via_action_a(void) {
    GameState g;
    game_init(&g);
    g.settings.match     = MATCH_PVA;
    g.settings.ai_player = PLAYER_WHITE;
    g.settings.ai_level  = 3; /* depth 1, quick */
    game_reset(&g);

    /* Black (player) moves → AI's turn, then AI plays → back to black. */
    g.cursor_x = 5; g.cursor_y = 2;
    game_action_a(&g);
    g.cursor_x = 6; g.cursor_y = 2;
    game_action_a(&g);
    ASSERT_EQ(g.mode, GAME_MODE_AI_THINKING);
    int max_steps = 20, steps = 0;
    while (!game_ai_step(&g) && steps < max_steps) steps++;
    ASSERT_EQ(g.board.current_player, PLAYER_BLACK);

    /* Scrub back to turn 1 (white = AI's turn) and place the cursor on a
     * white piece so the old fall-through would have selected it. */
    g.review_offset = -1;
    g.cursor_x = 5; g.cursor_y = 8;
    game_action_a(&g);

    ASSERT_EQ(g.board.turn_count, 1);
    ASSERT_EQ(g.board.current_player, PLAYER_WHITE);
    ASSERT_EQ(g.mode, GAME_MODE_AI_THINKING);
    ASSERT_TRUE(g.ai_pending);
    ASSERT_EQ(g.selected_x, -1);
    ASSERT_EQ(g.selected_y, -1);

    /* AI should be able to step to completion. */
    steps = 0;
    while (!game_ai_step(&g) && steps < max_steps) steps++;
    ASSERT_TRUE(steps < max_steps);
    ASSERT_FALSE(g.ai_pending);
    ASSERT_EQ(g.board.current_player, PLAYER_BLACK);
}

/* Commit to a snapshot of the player's own turn: A both commits AND selects
 * the piece under the cursor (the legacy convenience path). */
static void test_commit_review_to_player_turn_selects_piece(void) {
    GameState g;
    game_init(&g);
    g.settings.match = MATCH_PVP;
    game_reset(&g);
    play_two_moves(&g);

    /* Scrub back 1 turn → turn 1, white's turn in PvP. */
    g.review_offset = -1;
    g.cursor_x = 5; g.cursor_y = 8; /* white infantry exists here at turn 1 */
    game_action_a(&g);

    ASSERT_EQ(g.board.turn_count, 1);
    ASSERT_EQ(g.board.current_player, PLAYER_WHITE);
    ASSERT_EQ(g.mode, GAME_MODE_PIECE_SELECTED);
    ASSERT_EQ(g.selected_x, 5);
    ASSERT_EQ(g.selected_y, 8);
    ASSERT_TRUE(g.num_valid_moves > 0);
}

/* Commit while the cursor is over an empty tile in the past state — should
 * leave the player cleanly in FREE mode with the past state live, no stale
 * selection, and the hover preview refreshed (no highlights since cursor is
 * on an empty tile). */
static void test_commit_review_with_empty_cursor_just_commits(void) {
    GameState g;
    game_init(&g);
    g.settings.match = MATCH_PVP;
    game_reset(&g);
    play_two_moves(&g);

    g.review_offset = -1;
    g.cursor_x = 5; g.cursor_y = 5; /* empty middle tile */
    game_action_a(&g);

    ASSERT_EQ(g.review_offset, 0);
    ASSERT_EQ(g.mode, GAME_MODE_FREE);
    ASSERT_EQ(g.selected_x, -1);
    ASSERT_EQ(g.board.tiles[5][5].highlight, HIGHLIGHT_NONE);
}

/* Hover preview is refreshed after commit — cursor on a piece in the past
 * state shows that piece's move highlights, even though commit was triggered
 * from outside game_action_a (so the legacy "A also selects" path doesn't
 * run). */
static void test_commit_review_refreshes_hover(void) {
    GameState g;
    game_init(&g);
    g.settings.match = MATCH_PVP;
    game_reset(&g);
    play_two_moves(&g);

    g.review_offset = -1; /* turn 1, white's turn */
    g.cursor_x = 5; g.cursor_y = 8; /* white infantry */
    game_commit_review_branch(&g);

    ASSERT_EQ(g.mode, GAME_MODE_FREE);
    ASSERT_EQ(g.board.tiles[5][8].highlight, HIGHLIGHT_SELECTED);
    /* White infantry at (5,8) can step west to (4,8) on the default map. */
    ASSERT_EQ(g.board.tiles[4][8].highlight, HIGHLIGHT_MOVE);
}

static void test_ai_step_eventually_finishes(void) {
    GameState g;
    game_init(&g);
    g.settings.match     = MATCH_PVA;
    g.settings.ai_player = PLAYER_WHITE;
    g.settings.ai_level  = 5; /* depth 2 */
    game_reset(&g);

    /* Black moves so it becomes white (AI)'s turn. */
    g.cursor_x = 5; g.cursor_y = 2;
    game_action_a(&g);
    g.cursor_x = 6; g.cursor_y = 2;
    game_action_a(&g);

    ASSERT_EQ(g.mode, GAME_MODE_AI_THINKING);
    ASSERT_TRUE(g.ai_pending);

    /* Step until done. Should converge in <= target_depth + 1 calls. */
    int max_steps = 20;
    int steps = 0;
    while (!game_ai_step(&g) && steps < max_steps) steps++;
    ASSERT_TRUE(steps < max_steps);
    ASSERT_FALSE(g.ai_pending);
    ASSERT_EQ(g.board.current_player, PLAYER_BLACK);
    ASSERT_TRUE(g.mode == GAME_MODE_FREE || g.mode == GAME_MODE_GAME_OVER);
}

static void test_ai_step_walks_depths_in_order(void) {
    GameState g;
    game_init(&g);
    g.settings.match     = MATCH_PVA;
    g.settings.ai_player = PLAYER_WHITE;
    g.settings.ai_level  = 6; /* depth 2, no noise */
    game_reset(&g);

    g.cursor_x = 5; g.cursor_y = 2;
    game_action_a(&g);
    g.cursor_x = 6; g.cursor_y = 2;
    game_action_a(&g);

    /* First step initializes and runs depth 1. */
    bool done1 = game_ai_step(&g);
    ASSERT_FALSE(done1);
    ASSERT_EQ(g.ai_target_depth, 2);
    ASSERT_EQ(g.ai_next_depth, 2);
    ASSERT_TRUE(g.ai_has_best);

    /* Second step runs depth 2 and commits. */
    bool done2 = game_ai_step(&g);
    ASSERT_TRUE(done2);
    ASSERT_FALSE(g.ai_pending);
}

static int count_highlights_other_than_selected(const Board* b) {
    int n = 0;
    for (int x = 0; x < BOARD_DIM; x++) {
        for (int y = 0; y < BOARD_DIM; y++) {
            TileHighlight h = b->tiles[x][y].highlight;
            if (h == HIGHLIGHT_MOVE || h == HIGHLIGHT_ATTACK) n++;
        }
    }
    return n;
}

static void test_hover_shows_own_piece_moves(void) {
    GameState g;
    game_init(&g);
    game_reset(&g);
    /* Cursor on black infantry (5,2) — own piece, black's turn. */
    g.cursor_x = 5; g.cursor_y = 2;
    game_refresh_highlights(&g);
    /* It has at least one legal hop east to (6,2). */
    ASSERT_EQ(g.board.tiles[6][2].highlight, HIGHLIGHT_MOVE);
    ASSERT_TRUE(count_highlights_other_than_selected(&g.board) > 0);
}

static void test_hover_shows_enemy_piece_moves(void) {
    GameState g;
    game_init(&g);
    game_reset(&g);
    /* Cursor on white infantry (5,8). Black's turn — but preview should still
     * show what THAT piece could do. */
    g.cursor_x = 5; g.cursor_y = 8;
    game_refresh_highlights(&g);
    /* The white infantry can step west to (4,8). */
    ASSERT_EQ(g.board.tiles[4][8].highlight, HIGHLIGHT_MOVE);
    ASSERT_TRUE(count_highlights_other_than_selected(&g.board) > 0);
}

static void test_hover_clears_when_moving_off_piece(void) {
    GameState g;
    game_init(&g);
    game_reset(&g);
    g.cursor_x = 5; g.cursor_y = 2;
    game_refresh_highlights(&g);
    ASSERT_TRUE(count_highlights_other_than_selected(&g.board) > 0);

    /* Move cursor to an empty tile in the middle of the board. */
    g.cursor_x = 5; g.cursor_y = 5;
    game_refresh_highlights(&g);
    ASSERT_EQ(count_highlights_other_than_selected(&g.board), 0);
}

static void test_move_cursor_updates_hover(void) {
    GameState g;
    game_init(&g);
    game_reset(&g);
    /* Start over an empty tile. */
    g.cursor_x = 5; g.cursor_y = 5;
    game_refresh_highlights(&g);
    ASSERT_EQ(count_highlights_other_than_selected(&g.board), 0);

    /* Move cursor onto an own piece — hover preview should appear. */
    while (g.cursor_y > 2) game_move_cursor(&g, 0, -1);
    ASSERT_EQ(g.cursor_y, 2);
    ASSERT_TRUE(count_highlights_other_than_selected(&g.board) > 0);
}

static void test_ai_step_level_1_finishes_in_one_call(void) {
    GameState g;
    game_init(&g);
    g.settings.match     = MATCH_PVA;
    g.settings.ai_player = PLAYER_WHITE;
    g.settings.ai_level  = 3; /* depth 1 */
    game_reset(&g);

    g.cursor_x = 5; g.cursor_y = 2;
    game_action_a(&g);
    g.cursor_x = 6; g.cursor_y = 2;
    game_action_a(&g);

    bool done = game_ai_step(&g);
    ASSERT_TRUE(done);
    ASSERT_FALSE(g.ai_pending);
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
    RUN(test_commit_review_branch_truncates_future);
    RUN(test_commit_review_branch_noop_at_zero);
    RUN(test_commit_review_to_ai_turn_via_action_a);
    RUN(test_commit_review_to_player_turn_selects_piece);
    RUN(test_commit_review_with_empty_cursor_just_commits);
    RUN(test_commit_review_refreshes_hover);
    RUN(test_ai_step_eventually_finishes);
    RUN(test_ai_step_walks_depths_in_order);
    RUN(test_ai_step_level_1_finishes_in_one_call);
    RUN(test_hover_shows_own_piece_moves);
    RUN(test_hover_shows_enemy_piece_moves);
    RUN(test_hover_clears_when_moving_off_piece);
    RUN(test_move_cursor_updates_hover);
}
