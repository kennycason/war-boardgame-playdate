#include <string.h>
#include "test.h"
#include "board.h"

static void test_board_init_zeroes_state(void) {
    Board b;
    board_init(&b);
    ASSERT_EQ(b.turn_count, 0);
    ASSERT_EQ(b.black_score, 0);
    ASSERT_EQ(b.white_score, 0);
    ASSERT_EQ(b.win_state, WIN_NONE);
    ASSERT_EQ(b.current_player, PLAYER_BLACK);
    for (int x = 0; x < BOARD_DIM; x++) {
        for (int y = 0; y < BOARD_DIM; y++) {
            ASSERT_EQ(b.tiles[x][y].piece.type, PIECE_NONE);
            ASSERT_EQ(b.tiles[x][y].elevation, 0);
        }
    }
}

static void test_board_in_bounds(void) {
    ASSERT_TRUE(board_in_bounds(0, 0));
    ASSERT_TRUE(board_in_bounds(10, 10));
    ASSERT_TRUE(board_in_bounds(5, 5));
    ASSERT_FALSE(board_in_bounds(-1, 0));
    ASSERT_FALSE(board_in_bounds(0, -1));
    ASSERT_FALSE(board_in_bounds(11, 0));
    ASSERT_FALSE(board_in_bounds(0, 11));
}

static void test_board_setup_places_36_pieces(void) {
    Board b;
    board_init(&b);
    board_setup_pieces(&b);

    int total = 0, black = 0, white = 0;
    for (int x = 0; x < BOARD_DIM; x++) {
        for (int y = 0; y < BOARD_DIM; y++) {
            if (b.tiles[x][y].piece.type != PIECE_NONE) {
                total++;
                if (b.tiles[x][y].piece.player == PLAYER_BLACK) black++;
                if (b.tiles[x][y].piece.player == PLAYER_WHITE) white++;
            }
        }
    }
    ASSERT_EQ(total, 36);
    ASSERT_EQ(black, 18);
    ASSERT_EQ(white, 18);
}

static void test_board_setup_specific_pieces(void) {
    Board b;
    board_init(&b);
    board_setup_pieces(&b);
    ASSERT_EQ(b.tiles[0][0].piece.type, PIECE_COMMANDER);
    ASSERT_EQ(b.tiles[0][0].piece.player, PLAYER_BLACK);
    ASSERT_EQ(b.tiles[10][10].piece.type, PIECE_COMMANDER);
    ASSERT_EQ(b.tiles[10][10].piece.player, PLAYER_WHITE);
    ASSERT_EQ(b.tiles[1][1].piece.type, PIECE_AIR_DEFENSE);
    ASSERT_EQ(b.tiles[9][9].piece.type, PIECE_AIR_DEFENSE);
    ASSERT_EQ(b.tiles[3][0].piece.type, PIECE_TANK);
    ASSERT_EQ(b.tiles[7][10].piece.type, PIECE_TANK);
}

static void test_board_terrain_matches_kotlin_default(void) {
    Board b;
    board_init(&b);
    board_setup_terrain(&b, MAP_DEFAULT);
    /* Spot-check a few known cells from DefaultTerrainV2Generator. */
    ASSERT_EQ(b.tiles[0][0].elevation,  2);
    ASSERT_EQ(b.tiles[0][4].elevation,  3);
    ASSERT_EQ(b.tiles[5][3].elevation,  3);
    ASSERT_EQ(b.tiles[5][5].elevation,  3);
    ASSERT_EQ(b.tiles[10][10].elevation, 2);
    ASSERT_EQ(b.tiles[10][5].elevation,  3);
    /* All elevations within [0, MAX_ELEVATION]. */
    for (int x = 0; x < BOARD_DIM; x++) {
        for (int y = 0; y < BOARD_DIM; y++) {
            int e = b.tiles[x][y].elevation;
            ASSERT_TRUE(e >= 0 && e <= MAX_ELEVATION);
        }
    }
}

static void test_board_check_win_commander_dead(void) {
    Board b;
    board_init(&b);
    board_setup_pieces(&b);
    ASSERT_EQ(board_check_win(&b), WIN_NONE);
    /* Kill white's commander */
    b.tiles[10][10].piece.type = PIECE_NONE;
    ASSERT_EQ(board_check_win(&b), WIN_BLACK);
}

static void test_board_check_win_no_attack_pieces(void) {
    Board b;
    board_init(&b);
    /* Both commanders alive, but white has no attackers */
    b.tiles[0][0].piece.type   = PIECE_COMMANDER;
    b.tiles[0][0].piece.player = PLAYER_BLACK;
    b.tiles[1][0].piece.type   = PIECE_INFANTRY;
    b.tiles[1][0].piece.player = PLAYER_BLACK;
    b.tiles[10][10].piece.type   = PIECE_COMMANDER;
    b.tiles[10][10].piece.player = PLAYER_WHITE;
    ASSERT_EQ(board_check_win(&b), WIN_BLACK); /* white has no attackers */
}

static void test_board_clear_highlights(void) {
    Board b;
    board_init(&b);
    b.tiles[3][3].highlight = HIGHLIGHT_SELECTED;
    b.tiles[4][3].highlight = HIGHLIGHT_MOVE;
    b.tiles[3][4].highlight = HIGHLIGHT_ATTACK;
    board_clear_highlights(&b);
    for (int x = 0; x < BOARD_DIM; x++) {
        for (int y = 0; y < BOARD_DIM; y++) {
            ASSERT_EQ(b.tiles[x][y].highlight, HIGHLIGHT_NONE);
        }
    }
}

static void test_board_terrain_all_maps_in_range(void) {
    Board b;
    MapLayout maps[] = { MAP_DEFAULT, MAP_CLASSIC, MAP_VALLEY, MAP_WAVES, MAP_RANDOM };
    for (size_t i = 0; i < sizeof(maps)/sizeof(maps[0]); i++) {
        board_init(&b);
        board_setup_terrain(&b, maps[i]);
        for (int x = 0; x < BOARD_DIM; x++) {
            for (int y = 0; y < BOARD_DIM; y++) {
                int e = b.tiles[x][y].elevation;
                ASSERT_TRUE(e >= MIN_ELEVATION && e <= MAX_ELEVATION);
            }
        }
    }
}

static void test_board_map_names(void) {
    ASSERT_TRUE(strcmp(board_map_name(MAP_DEFAULT), "DEFAULT") == 0);
    ASSERT_TRUE(strcmp(board_map_name(MAP_CLASSIC), "CLASSIC") == 0);
    ASSERT_TRUE(strcmp(board_map_name(MAP_VALLEY),  "VALLEY") == 0);
    ASSERT_TRUE(strcmp(board_map_name(MAP_WAVES),   "WAVES") == 0);
    ASSERT_TRUE(strcmp(board_map_name(MAP_RANDOM),  "RANDOM") == 0);
}

void run_board_tests(void) {
    SECTION("Board");
    RUN(test_board_init_zeroes_state);
    RUN(test_board_in_bounds);
    RUN(test_board_setup_places_36_pieces);
    RUN(test_board_setup_specific_pieces);
    RUN(test_board_terrain_matches_kotlin_default);
    RUN(test_board_check_win_commander_dead);
    RUN(test_board_check_win_no_attack_pieces);
    RUN(test_board_clear_highlights);
    RUN(test_board_terrain_all_maps_in_range);
    RUN(test_board_map_names);
}
