#include <string.h>
#include "test.h"
#include "board.h"
#include "move.h"

/* Helpers */

static int count_move_type(const Move* moves, int n, MoveType mt) {
    int c = 0;
    for (int i = 0; i < n; i++) if (moves[i].move_type == mt) c++;
    return c;
}

static bool has_move(const Move* moves, int n, int tx, int ty, MoveType mt) {
    for (int i = 0; i < n; i++) {
        if (moves[i].to_x == tx && moves[i].to_y == ty && moves[i].move_type == mt) return true;
    }
    return false;
}

/* Always wipes the board so each test has full control over piece placement. */
static void setup_empty(Board* b) {
    board_init(b);
}

/* ---------- Infantry ---------- */

static void test_infantry_open_board(void) {
    Board b; setup_empty(&b);
    board_place_piece(&b, PIECE_INFANTRY, PLAYER_BLACK, 5, 5);

    Move moves[MAX_MOVES_PER_PIECE];
    int n = generate_moves(&b, 5, 5, moves, MAX_MOVES_PER_PIECE);
    /* 4 cardinal moves, 0 attacks (no enemies). */
    ASSERT_EQ(n, 4);
    ASSERT_EQ(count_move_type(moves, n, MOVE_TYPE_MOVE), 4);
    ASSERT_TRUE(has_move(moves, n, 4, 5, MOVE_TYPE_MOVE));
    ASSERT_TRUE(has_move(moves, n, 6, 5, MOVE_TYPE_MOVE));
    ASSERT_TRUE(has_move(moves, n, 5, 4, MOVE_TYPE_MOVE));
    ASSERT_TRUE(has_move(moves, n, 5, 6, MOVE_TYPE_MOVE));
}

static void test_infantry_only_attacks_diagonally(void) {
    Board b; setup_empty(&b);
    board_place_piece(&b, PIECE_INFANTRY, PLAYER_BLACK, 5, 5);
    /* enemy directly N, S, E, W — must NOT generate attack moves */
    board_place_piece(&b, PIECE_INFANTRY, PLAYER_WHITE, 4, 5);
    board_place_piece(&b, PIECE_INFANTRY, PLAYER_WHITE, 5, 4);
    /* enemy diagonal — MUST generate attack moves */
    board_place_piece(&b, PIECE_INFANTRY, PLAYER_WHITE, 6, 6);
    board_place_piece(&b, PIECE_INFANTRY, PLAYER_WHITE, 4, 6);

    Move moves[MAX_MOVES_PER_PIECE];
    int n = generate_moves(&b, 5, 5, moves, MAX_MOVES_PER_PIECE);
    /* HV moves blocked by enemies in those cells (but cannot attack them).
     * 2 free HV moves (N=5,6 / E=6,5).  2 diagonal attacks. */
    ASSERT_EQ(count_move_type(moves, n, MOVE_TYPE_MOVE), 2);
    ASSERT_EQ(count_move_type(moves, n, MOVE_TYPE_ATTACK), 2);
    ASSERT_TRUE(has_move(moves, n, 6, 6, MOVE_TYPE_ATTACK));
    ASSERT_TRUE(has_move(moves, n, 4, 6, MOVE_TYPE_ATTACK));
    ASSERT_FALSE(has_move(moves, n, 4, 5, MOVE_TYPE_ATTACK));
    ASSERT_FALSE(has_move(moves, n, 5, 4, MOVE_TYPE_ATTACK));
}

/* ---------- Tank ---------- */

static void test_tank_open_board(void) {
    Board b; setup_empty(&b);
    board_place_piece(&b, PIECE_TANK, PLAYER_BLACK, 5, 5);
    Move moves[MAX_MOVES_PER_PIECE];
    int n = generate_moves(&b, 5, 5, moves, MAX_MOVES_PER_PIECE);
    /* 2 cells in 4 directions = 8 moves. */
    ASSERT_EQ(n, 8);
}

static void test_tank_blocked_by_own_piece(void) {
    Board b; setup_empty(&b);
    board_place_piece(&b, PIECE_TANK, PLAYER_BLACK, 5, 5);
    board_place_piece(&b, PIECE_INFANTRY, PLAYER_BLACK, 6, 5);
    Move moves[MAX_MOVES_PER_PIECE];
    int n = generate_moves(&b, 5, 5, moves, MAX_MOVES_PER_PIECE);
    /* Right direction blocked by own piece at 6,5 → 0 moves right.
     * Other 3 dirs: 2 each = 6.  Total 6. */
    ASSERT_EQ(n, 6);
    ASSERT_FALSE(has_move(moves, n, 6, 5, MOVE_TYPE_MOVE));
    ASSERT_FALSE(has_move(moves, n, 7, 5, MOVE_TYPE_MOVE));
}

static void test_tank_attacks_enemy(void) {
    Board b; setup_empty(&b);
    board_place_piece(&b, PIECE_TANK, PLAYER_BLACK, 5, 5);
    board_place_piece(&b, PIECE_INFANTRY, PLAYER_WHITE, 7, 5);
    Move moves[MAX_MOVES_PER_PIECE];
    int n = generate_moves(&b, 5, 5, moves, MAX_MOVES_PER_PIECE);
    /* Right: move to 6,5 + attack 7,5. Then blocked.
     * Other 3 dirs: 2 moves = 6.  Total 8. */
    ASSERT_EQ(n, 8);
    ASSERT_TRUE(has_move(moves, n, 6, 5, MOVE_TYPE_MOVE));
    ASSERT_TRUE(has_move(moves, n, 7, 5, MOVE_TYPE_ATTACK));
}

/* ---------- Sniper ---------- */

static void test_sniper_diagonal_only(void) {
    Board b; setup_empty(&b);
    board_place_piece(&b, PIECE_SNIPER, PLAYER_BLACK, 5, 5);
    Move moves[MAX_MOVES_PER_PIECE];
    int n = generate_moves(&b, 5, 5, moves, MAX_MOVES_PER_PIECE);
    ASSERT_EQ(n, 8); /* 2 cells × 4 diag dirs */
    /* All targets should be diagonal */
    for (int i = 0; i < n; i++) {
        int dx = moves[i].to_x - 5;
        int dy = moves[i].to_y - 5;
        if (dx < 0) dx = -dx;
        if (dy < 0) dy = -dy;
        ASSERT_EQ(dx, dy);
    }
}

/* ---------- Commander ---------- */

static void test_commander_8_directions(void) {
    Board b; setup_empty(&b);
    board_place_piece(&b, PIECE_COMMANDER, PLAYER_BLACK, 5, 5);
    Move moves[MAX_MOVES_PER_PIECE];
    int n = generate_moves(&b, 5, 5, moves, MAX_MOVES_PER_PIECE);
    ASSERT_EQ(n, 8); /* 1 cell × 8 dirs */
}

/* ---------- Artillery ---------- */

static void test_artillery_open_board(void) {
    Board b; setup_empty(&b);
    board_place_piece(&b, PIECE_ARTILLERY, PLAYER_BLACK, 5, 5);
    Move moves[MAX_MOVES_PER_PIECE];
    int n = generate_moves(&b, 5, 5, moves, MAX_MOVES_PER_PIECE);
    /* 8 move dirs (HV+diag at dist 1), no targets → no attacks. */
    ASSERT_EQ(n, 8);
    ASSERT_EQ(count_move_type(moves, n, MOVE_TYPE_ATTACK), 0);
}

static void test_artillery_attacks_distant_enemy(void) {
    Board b; setup_empty(&b);
    board_place_piece(&b, PIECE_ARTILLERY, PLAYER_BLACK, 5, 5);
    board_place_piece(&b, PIECE_INFANTRY,  PLAYER_WHITE, 7, 5); /* 2 tiles east */
    board_place_piece(&b, PIECE_TANK,      PLAYER_WHITE, 8, 5); /* 3 tiles east */
    Move moves[MAX_MOVES_PER_PIECE];
    int n = generate_moves(&b, 5, 5, moves, MAX_MOVES_PER_PIECE);
    ASSERT_TRUE(has_move(moves, n, 7, 5, MOVE_TYPE_ATTACK));
    ASSERT_TRUE(has_move(moves, n, 8, 5, MOVE_TYPE_ATTACK));
    /* Artillery cannot attack at range 1, only its 8 movement squares. */
    ASSERT_FALSE(has_move(moves, n, 6, 5, MOVE_TYPE_ATTACK));
}

static void test_artillery_ignores_elevation_for_attack(void) {
    Board b; setup_empty(&b);
    board_place_piece(&b, PIECE_ARTILLERY, PLAYER_BLACK, 5, 5);
    board_place_piece(&b, PIECE_INFANTRY,  PLAYER_WHITE, 7, 5);
    b.tiles[6][5].elevation = 4; /* would block normal HV move */
    Move moves[MAX_MOVES_PER_PIECE];
    int n = generate_moves(&b, 5, 5, moves, MAX_MOVES_PER_PIECE);
    ASSERT_TRUE(has_move(moves, n, 7, 5, MOVE_TYPE_ATTACK));
}

static void test_artillery_reload_blocks_attack(void) {
    Board b; setup_empty(&b);
    board_place_piece(&b, PIECE_ARTILLERY, PLAYER_BLACK, 5, 5);
    board_place_piece(&b, PIECE_INFANTRY,  PLAYER_WHITE, 7, 5);
    b.tiles[5][5].piece.reloading = true;
    b.tiles[5][5].piece.last_attack_turn = b.turn_count; /* just attacked */
    Move moves[MAX_MOVES_PER_PIECE];
    int n = generate_moves(&b, 5, 5, moves, MAX_MOVES_PER_PIECE);
    ASSERT_EQ(count_move_type(moves, n, MOVE_TYPE_ATTACK), 0);
}

static void test_artillery_reload_clears_after_turns(void) {
    Board b; setup_empty(&b);
    board_place_piece(&b, PIECE_ARTILLERY, PLAYER_BLACK, 5, 5);
    board_place_piece(&b, PIECE_INFANTRY,  PLAYER_WHITE, 7, 5);
    b.tiles[5][5].piece.reloading = true;
    b.tiles[5][5].piece.last_attack_turn = 0;
    b.turn_count = ARTILLERY_RELOAD_TURNS + 1;
    Move moves[MAX_MOVES_PER_PIECE];
    int n = generate_moves(&b, 5, 5, moves, MAX_MOVES_PER_PIECE);
    ASSERT_FALSE(b.tiles[5][5].piece.reloading);
    ASSERT_TRUE(count_move_type(moves, n, MOVE_TYPE_ATTACK) > 0);
}

/* ---------- Missile ---------- */

static void test_missile_attacks_diagonally(void) {
    Board b; setup_empty(&b);
    board_place_piece(&b, PIECE_MISSILE, PLAYER_BLACK, 5, 5);
    board_place_piece(&b, PIECE_INFANTRY, PLAYER_WHITE, 8, 8); /* 3 tiles diag */
    Move moves[MAX_MOVES_PER_PIECE];
    int n = generate_moves(&b, 5, 5, moves, MAX_MOVES_PER_PIECE);
    ASSERT_TRUE(has_move(moves, n, 8, 8, MOVE_TYPE_ATTACK));
}

static void test_missile_cannot_attack_horizontally(void) {
    Board b; setup_empty(&b);
    board_place_piece(&b, PIECE_MISSILE, PLAYER_BLACK, 5, 5);
    board_place_piece(&b, PIECE_INFANTRY, PLAYER_WHITE, 7, 5);
    Move moves[MAX_MOVES_PER_PIECE];
    int n = generate_moves(&b, 5, 5, moves, MAX_MOVES_PER_PIECE);
    ASSERT_FALSE(has_move(moves, n, 7, 5, MOVE_TYPE_ATTACK));
}

/* ---------- Air Defense ---------- */

static void test_air_defense_no_attacks(void) {
    Board b; setup_empty(&b);
    board_place_piece(&b, PIECE_AIR_DEFENSE, PLAYER_BLACK, 5, 5);
    board_place_piece(&b, PIECE_INFANTRY,    PLAYER_WHITE, 6, 5);
    Move moves[MAX_MOVES_PER_PIECE];
    int n = generate_moves(&b, 5, 5, moves, MAX_MOVES_PER_PIECE);
    ASSERT_EQ(count_move_type(moves, n, MOVE_TYPE_ATTACK), 0);
}

/* ---------- Bomber ---------- */

static void test_bomber_4_squares_4_dirs(void) {
    Board b; setup_empty(&b);
    board_place_piece(&b, PIECE_BOMBER, PLAYER_BLACK, 5, 5);
    Move moves[MAX_MOVES_PER_PIECE];
    int n = generate_moves(&b, 5, 5, moves, MAX_MOVES_PER_PIECE);
    ASSERT_EQ(n, 16); /* 4 dirs × 4 squares */
}

static void test_bomber_flies_over_own_pieces(void) {
    Board b; setup_empty(&b);
    board_place_piece(&b, PIECE_BOMBER,   PLAYER_BLACK, 5, 5);
    board_place_piece(&b, PIECE_INFANTRY, PLAYER_BLACK, 6, 5);
    Move moves[MAX_MOVES_PER_PIECE];
    int n = generate_moves(&b, 5, 5, moves, MAX_MOVES_PER_PIECE);
    /* east direction passes own infantry → can still reach 7,5 / 8,5 / 9,5 */
    ASSERT_TRUE(has_move(moves, n, 7, 5, MOVE_TYPE_MOVE));
    ASSERT_TRUE(has_move(moves, n, 8, 5, MOVE_TYPE_MOVE));
    /* cannot land on own infantry */
    ASSERT_FALSE(has_move(moves, n, 6, 5, MOVE_TYPE_MOVE));
}

static void test_bomber_blocked_by_enemy(void) {
    Board b; setup_empty(&b);
    board_place_piece(&b, PIECE_BOMBER,   PLAYER_BLACK, 5, 5);
    board_place_piece(&b, PIECE_INFANTRY, PLAYER_WHITE, 6, 5);
    Move moves[MAX_MOVES_PER_PIECE];
    int n = generate_moves(&b, 5, 5, moves, MAX_MOVES_PER_PIECE);
    /* east: attack 6,5, but cannot pass enemy */
    ASSERT_TRUE(has_move(moves, n, 6, 5, MOVE_TYPE_ATTACK));
    ASSERT_FALSE(has_move(moves, n, 7, 5, MOVE_TYPE_MOVE));
    ASSERT_FALSE(has_move(moves, n, 7, 5, MOVE_TYPE_ATTACK));
}

static void test_bomber_ignores_elevation(void) {
    Board b; setup_empty(&b);
    board_place_piece(&b, PIECE_BOMBER, PLAYER_BLACK, 5, 5);
    b.tiles[6][5].elevation = 4;
    Move moves[MAX_MOVES_PER_PIECE];
    int n = generate_moves(&b, 5, 5, moves, MAX_MOVES_PER_PIECE);
    ASSERT_TRUE(has_move(moves, n, 6, 5, MOVE_TYPE_MOVE));
    ASSERT_TRUE(has_move(moves, n, 7, 5, MOVE_TYPE_MOVE));
}

/* ---------- Elevation movement ---------- */

static void test_tank_blocked_by_elevation(void) {
    Board b; setup_empty(&b);
    board_place_piece(&b, PIECE_TANK, PLAYER_BLACK, 5, 5);
    b.tiles[6][5].elevation = 2; /* +2 climb is blocked */
    Move moves[MAX_MOVES_PER_PIECE];
    int n = generate_moves(&b, 5, 5, moves, MAX_MOVES_PER_PIECE);
    ASSERT_FALSE(has_move(moves, n, 6, 5, MOVE_TYPE_MOVE));
}

static void test_tank_can_climb_gentle_slope(void) {
    Board b; setup_empty(&b);
    board_place_piece(&b, PIECE_TANK, PLAYER_BLACK, 5, 5);
    b.tiles[6][5].elevation = 1;
    Move moves[MAX_MOVES_PER_PIECE];
    int n = generate_moves(&b, 5, 5, moves, MAX_MOVES_PER_PIECE);
    ASSERT_TRUE(has_move(moves, n, 6, 5, MOVE_TYPE_MOVE));
}

/* ---------- Excavator ---------- */

static void test_excavator_generates_raise_and_lower(void) {
    Board b; setup_empty(&b);
    board_place_piece(&b, PIECE_EXCAVATOR, PLAYER_BLACK, 5, 5);
    /* Adjacent tile must be reachable (elev diff ≤ 1) for excavator to act on it. */
    b.tiles[6][5].elevation = 1;
    Move moves[MAX_MOVES_PER_PIECE];
    int n = generate_moves(&b, 5, 5, moves, MAX_MOVES_PER_PIECE);
    bool found_raise = false, found_lower = false;
    for (int i = 0; i < n; i++) {
        if (moves[i].to_x == 6 && moves[i].to_y == 5) {
            if (moves[i].elevation_delta == +1) found_raise = true;
            if (moves[i].elevation_delta == -1) found_lower = true;
        }
    }
    ASSERT_TRUE(found_raise);
    ASSERT_TRUE(found_lower);
}

/* ---------- apply_move ---------- */

static void test_apply_move_standard(void) {
    Board b; setup_empty(&b);
    board_place_piece(&b, PIECE_INFANTRY, PLAYER_BLACK, 5, 5);
    Move m;
    ASSERT_TRUE(find_move(&b, 5, 5, 5, 6, 0, &m));
    apply_move(&b, &m);
    ASSERT_EQ(b.tiles[5][5].piece.type, PIECE_NONE);
    ASSERT_EQ(b.tiles[5][6].piece.type, PIECE_INFANTRY);
    ASSERT_EQ(b.current_player, PLAYER_WHITE);
    ASSERT_EQ(b.turn_count, 1);
}

static void test_apply_attack_increases_score(void) {
    Board b; setup_empty(&b);
    board_place_piece(&b, PIECE_TANK,     PLAYER_BLACK, 5, 5);
    board_place_piece(&b, PIECE_INFANTRY, PLAYER_WHITE, 6, 5);
    Move m;
    ASSERT_TRUE(find_move(&b, 5, 5, 6, 5, 0, &m));
    ASSERT_EQ(m.move_type, MOVE_TYPE_ATTACK);
    apply_move(&b, &m);
    ASSERT_EQ(b.black_score, 1);
    ASSERT_EQ(b.tiles[6][5].piece.type, PIECE_TANK);
    ASSERT_EQ(b.tiles[6][5].piece.player, PLAYER_BLACK);
}

static void test_apply_artillery_attack_stays_in_place(void) {
    Board b; setup_empty(&b);
    board_place_piece(&b, PIECE_ARTILLERY, PLAYER_BLACK, 5, 5);
    board_place_piece(&b, PIECE_INFANTRY,  PLAYER_WHITE, 7, 5);
    Move m;
    ASSERT_TRUE(find_move(&b, 5, 5, 7, 5, 0, &m));
    apply_move(&b, &m);
    ASSERT_EQ(b.tiles[5][5].piece.type, PIECE_ARTILLERY);
    ASSERT_EQ(b.tiles[7][5].piece.type, PIECE_NONE);
    ASSERT_TRUE(b.tiles[5][5].piece.reloading);
}

static void test_apply_missile_self_destructs(void) {
    Board b; setup_empty(&b);
    board_place_piece(&b, PIECE_MISSILE,  PLAYER_BLACK, 5, 5);
    board_place_piece(&b, PIECE_INFANTRY, PLAYER_WHITE, 8, 8);
    Move m;
    ASSERT_TRUE(find_move(&b, 5, 5, 8, 8, 0, &m));
    apply_move(&b, &m);
    ASSERT_EQ(b.tiles[5][5].piece.type, PIECE_NONE);
    ASSERT_EQ(b.tiles[8][8].piece.type, PIECE_NONE);
    ASSERT_EQ(b.black_score, 1);
}

static void test_apply_missile_intercepted_by_air_defense(void) {
    Board b; setup_empty(&b);
    board_place_piece(&b, PIECE_MISSILE,     PLAYER_BLACK, 5, 5);
    board_place_piece(&b, PIECE_INFANTRY,    PLAYER_WHITE, 8, 8);
    board_place_piece(&b, PIECE_AIR_DEFENSE, PLAYER_WHITE, 7, 8); /* adjacent to target */
    Move m;
    ASSERT_TRUE(find_move(&b, 5, 5, 8, 8, 0, &m));
    apply_move(&b, &m);
    ASSERT_EQ(b.tiles[5][5].piece.type, PIECE_NONE); /* missile gone */
    ASSERT_EQ(b.tiles[7][8].piece.type, PIECE_NONE); /* AD destroyed */
    ASSERT_EQ(b.tiles[8][8].piece.type, PIECE_INFANTRY); /* target survives */
    ASSERT_EQ(b.black_score, 0);
    ASSERT_TRUE(m.intercepted);
}

static void test_apply_bomber_intercepted_on_attack(void) {
    Board b; setup_empty(&b);
    board_place_piece(&b, PIECE_BOMBER,      PLAYER_BLACK, 0, 5);
    board_place_piece(&b, PIECE_INFANTRY,    PLAYER_WHITE, 3, 5);
    board_place_piece(&b, PIECE_AIR_DEFENSE, PLAYER_WHITE, 4, 5);
    Move m;
    ASSERT_TRUE(find_move(&b, 0, 5, 3, 5, 0, &m));
    apply_move(&b, &m);
    ASSERT_EQ(b.tiles[0][5].piece.type, PIECE_NONE);          /* bomber gone */
    ASSERT_EQ(b.tiles[4][5].piece.type, PIECE_NONE);          /* AD gone */
    ASSERT_EQ(b.tiles[3][5].piece.type, PIECE_INFANTRY);      /* target survives */
}

static void test_apply_bomber_intercepted_on_move(void) {
    Board b; setup_empty(&b);
    board_place_piece(&b, PIECE_BOMBER,      PLAYER_BLACK, 0, 5);
    board_place_piece(&b, PIECE_AIR_DEFENSE, PLAYER_WHITE, 4, 5);
    Move m;
    /* bomber tries to move to 3,5 — adjacent to enemy AD */
    ASSERT_TRUE(find_move(&b, 0, 5, 3, 5, 0, &m));
    apply_move(&b, &m);
    ASSERT_EQ(b.tiles[0][5].piece.type, PIECE_NONE);
    ASSERT_EQ(b.tiles[4][5].piece.type, PIECE_NONE);
    ASSERT_EQ(b.tiles[3][5].piece.type, PIECE_NONE);
}

static void test_apply_excavator_raises_elevation(void) {
    Board b; setup_empty(&b);
    board_place_piece(&b, PIECE_EXCAVATOR, PLAYER_BLACK, 5, 5);
    b.tiles[6][5].elevation = 1;
    Move m;
    ASSERT_TRUE(find_move(&b, 5, 5, 6, 5, +1, &m));
    apply_move(&b, &m);
    ASSERT_EQ(b.tiles[6][5].elevation, 2);
    ASSERT_EQ(b.tiles[6][5].piece.type, PIECE_EXCAVATOR);
}

static void test_apply_win_state_set_on_commander_capture(void) {
    Board b; setup_empty(&b);
    board_place_piece(&b, PIECE_TANK,      PLAYER_BLACK, 5, 5);
    board_place_piece(&b, PIECE_COMMANDER, PLAYER_WHITE, 6, 5);
    board_place_piece(&b, PIECE_COMMANDER, PLAYER_BLACK, 0, 0);
    /* Give white some attack piece so it doesn't auto-lose from
     * the "no attackers" rule before its commander dies. */
    board_place_piece(&b, PIECE_INFANTRY,  PLAYER_WHITE, 10, 10);
    Move m;
    ASSERT_TRUE(find_move(&b, 5, 5, 6, 5, 0, &m));
    apply_move(&b, &m);
    ASSERT_EQ(b.win_state, WIN_BLACK);
    ASSERT_EQ(b.black_score, COMMANDER_SCORE);
}

void run_move_tests(void) {
    SECTION("Move generation & application");
    RUN(test_infantry_open_board);
    RUN(test_infantry_only_attacks_diagonally);
    RUN(test_tank_open_board);
    RUN(test_tank_blocked_by_own_piece);
    RUN(test_tank_attacks_enemy);
    RUN(test_sniper_diagonal_only);
    RUN(test_commander_8_directions);
    RUN(test_artillery_open_board);
    RUN(test_artillery_attacks_distant_enemy);
    RUN(test_artillery_ignores_elevation_for_attack);
    RUN(test_artillery_reload_blocks_attack);
    RUN(test_artillery_reload_clears_after_turns);
    RUN(test_missile_attacks_diagonally);
    RUN(test_missile_cannot_attack_horizontally);
    RUN(test_air_defense_no_attacks);
    RUN(test_bomber_4_squares_4_dirs);
    RUN(test_bomber_flies_over_own_pieces);
    RUN(test_bomber_blocked_by_enemy);
    RUN(test_bomber_ignores_elevation);
    RUN(test_tank_blocked_by_elevation);
    RUN(test_tank_can_climb_gentle_slope);
    RUN(test_excavator_generates_raise_and_lower);
    RUN(test_apply_move_standard);
    RUN(test_apply_attack_increases_score);
    RUN(test_apply_artillery_attack_stays_in_place);
    RUN(test_apply_missile_self_destructs);
    RUN(test_apply_missile_intercepted_by_air_defense);
    RUN(test_apply_bomber_intercepted_on_attack);
    RUN(test_apply_bomber_intercepted_on_move);
    RUN(test_apply_excavator_raises_elevation);
    RUN(test_apply_win_state_set_on_commander_capture);
}
