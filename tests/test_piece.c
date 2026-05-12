#include <string.h>
#include "test.h"
#include "piece.h"

static void test_piece_scores(void) {
    ASSERT_EQ(piece_score(PIECE_NONE), 0);
    ASSERT_EQ(piece_score(PIECE_INFANTRY), 1);
    ASSERT_EQ(piece_score(PIECE_TANK), 2);
    ASSERT_EQ(piece_score(PIECE_SNIPER), 2);
    ASSERT_EQ(piece_score(PIECE_ARTILLERY), 3);
    ASSERT_EQ(piece_score(PIECE_EXCAVATOR), 3);
    ASSERT_EQ(piece_score(PIECE_MISSILE), 4);
    ASSERT_EQ(piece_score(PIECE_AIR_DEFENSE), 4);
    ASSERT_EQ(piece_score(PIECE_BOMBER), 5);
    ASSERT_EQ(piece_score(PIECE_COMMANDER), 100);
}

static void test_piece_short_names(void) {
    ASSERT_TRUE(strcmp(piece_short_name(PIECE_INFANTRY), "INF") == 0);
    ASSERT_TRUE(strcmp(piece_short_name(PIECE_COMMANDER), "CMD") == 0);
    ASSERT_TRUE(strcmp(piece_short_name(PIECE_AIR_DEFENSE), "ADF") == 0);
    ASSERT_TRUE(strcmp(piece_short_name(PIECE_BOMBER), "BMB") == 0);
}

static void test_piece_letters_unique(void) {
    char seen[256];
    memset(seen, 0, sizeof(seen));
    PieceType all[] = {
        PIECE_INFANTRY, PIECE_TANK, PIECE_SNIPER, PIECE_ARTILLERY,
        PIECE_MISSILE, PIECE_AIR_DEFENSE, PIECE_BOMBER,
        PIECE_COMMANDER, PIECE_EXCAVATOR,
    };
    for (size_t i = 0; i < sizeof(all)/sizeof(all[0]); i++) {
        char c = piece_letter(all[i]);
        ASSERT_FALSE(seen[(unsigned char)c]);
        seen[(unsigned char)c] = 1;
    }
}

void run_piece_tests(void) {
    SECTION("Piece");
    RUN(test_piece_scores);
    RUN(test_piece_short_names);
    RUN(test_piece_letters_unique);
}
