#include <string.h>
#include "board.h"

void board_init(Board* b) {
    memset(b, 0, sizeof(Board));
    b->current_player = PLAYER_BLACK;
}

void board_reset(Board* b) {
    board_init(b);
    board_setup_terrain(b);
    board_setup_pieces(b);
}

bool board_in_bounds(int x, int y) {
    return x >= 0 && x < BOARD_DIM && y >= 0 && y < BOARD_DIM;
}

void board_place_piece(Board* b, PieceType type, Player player, int x, int y) {
    Piece* p = &b->tiles[x][y].piece;
    p->type             = type;
    p->player           = player;
    p->x                = x;
    p->y                = y;
    p->reloading        = false;
    p->last_attack_turn = 0;
}

void board_clear_highlights(Board* b) {
    for (int x = 0; x < BOARD_DIM; x++) {
        for (int y = 0; y < BOARD_DIM; y++) {
            b->tiles[x][y].highlight = HIGHLIGHT_NONE;
        }
    }
}

void board_setup_terrain(Board* b) {
    /* Symmetric hill pattern — peaks in the central region with a small crater
     * at the exact center. Designed so neither side gets a starting advantage. */
    static const int elev[BOARD_DIM][BOARD_DIM] = {
        {0,0,1,1,1,1,1,1,1,0,0},
        {0,1,1,2,2,2,2,2,1,1,0},
        {1,1,2,2,3,3,3,2,2,1,1},
        {1,2,2,3,3,3,3,3,2,2,1},
        {1,2,3,3,3,2,3,3,3,2,1},
        {1,2,3,3,2,1,2,3,3,2,1},
        {1,2,3,3,3,2,3,3,3,2,1},
        {1,2,2,3,3,3,3,3,2,2,1},
        {1,1,2,2,3,3,3,2,2,1,1},
        {0,1,1,2,2,2,2,2,1,1,0},
        {0,0,1,1,1,1,1,1,1,0,0},
    };
    for (int x = 0; x < BOARD_DIM; x++) {
        for (int y = 0; y < BOARD_DIM; y++) {
            b->tiles[x][y].elevation = elev[x][y];
        }
    }
}

void board_setup_pieces(Board* b) {
    /* BLACK: corner (0,0), filling x 0..5, y 0..2 */
    board_place_piece(b, PIECE_COMMANDER,   PLAYER_BLACK, 0, 0);
    board_place_piece(b, PIECE_MISSILE,     PLAYER_BLACK, 0, 1);
    board_place_piece(b, PIECE_SNIPER,      PLAYER_BLACK, 0, 2);
    board_place_piece(b, PIECE_BOMBER,      PLAYER_BLACK, 1, 0);
    board_place_piece(b, PIECE_AIR_DEFENSE, PLAYER_BLACK, 1, 1);
    board_place_piece(b, PIECE_INFANTRY,    PLAYER_BLACK, 1, 2);
    board_place_piece(b, PIECE_ARTILLERY,   PLAYER_BLACK, 2, 0);
    board_place_piece(b, PIECE_ARTILLERY,   PLAYER_BLACK, 2, 1);
    board_place_piece(b, PIECE_INFANTRY,    PLAYER_BLACK, 2, 2);
    board_place_piece(b, PIECE_TANK,        PLAYER_BLACK, 3, 0);
    board_place_piece(b, PIECE_TANK,        PLAYER_BLACK, 3, 1);
    board_place_piece(b, PIECE_INFANTRY,    PLAYER_BLACK, 3, 2);
    board_place_piece(b, PIECE_BOMBER,      PLAYER_BLACK, 4, 0);
    board_place_piece(b, PIECE_INFANTRY,    PLAYER_BLACK, 4, 1);
    board_place_piece(b, PIECE_INFANTRY,    PLAYER_BLACK, 4, 2);
    board_place_piece(b, PIECE_SNIPER,      PLAYER_BLACK, 5, 0);
    board_place_piece(b, PIECE_INFANTRY,    PLAYER_BLACK, 5, 1);
    board_place_piece(b, PIECE_INFANTRY,    PLAYER_BLACK, 5, 2);

    /* WHITE: corner (M,M), mirrored — x M..M-5, y M..M-2 */
    const int M = BOARD_DIM - 1;
    board_place_piece(b, PIECE_COMMANDER,   PLAYER_WHITE, M - 0, M - 0);
    board_place_piece(b, PIECE_MISSILE,     PLAYER_WHITE, M - 0, M - 1);
    board_place_piece(b, PIECE_SNIPER,      PLAYER_WHITE, M - 0, M - 2);
    board_place_piece(b, PIECE_BOMBER,      PLAYER_WHITE, M - 1, M - 0);
    board_place_piece(b, PIECE_AIR_DEFENSE, PLAYER_WHITE, M - 1, M - 1);
    board_place_piece(b, PIECE_INFANTRY,    PLAYER_WHITE, M - 1, M - 2);
    board_place_piece(b, PIECE_ARTILLERY,   PLAYER_WHITE, M - 2, M - 0);
    board_place_piece(b, PIECE_ARTILLERY,   PLAYER_WHITE, M - 2, M - 1);
    board_place_piece(b, PIECE_INFANTRY,    PLAYER_WHITE, M - 2, M - 2);
    board_place_piece(b, PIECE_TANK,        PLAYER_WHITE, M - 3, M - 0);
    board_place_piece(b, PIECE_TANK,        PLAYER_WHITE, M - 3, M - 1);
    board_place_piece(b, PIECE_INFANTRY,    PLAYER_WHITE, M - 3, M - 2);
    board_place_piece(b, PIECE_BOMBER,      PLAYER_WHITE, M - 4, M - 0);
    board_place_piece(b, PIECE_INFANTRY,    PLAYER_WHITE, M - 4, M - 1);
    board_place_piece(b, PIECE_INFANTRY,    PLAYER_WHITE, M - 4, M - 2);
    board_place_piece(b, PIECE_SNIPER,      PLAYER_WHITE, M - 5, M - 0);
    board_place_piece(b, PIECE_INFANTRY,    PLAYER_WHITE, M - 5, M - 1);
    board_place_piece(b, PIECE_INFANTRY,    PLAYER_WHITE, M - 5, M - 2);
}

int board_count_attack_pieces(const Board* b, Player player) {
    int count = 0;
    for (int x = 0; x < BOARD_DIM; x++) {
        for (int y = 0; y < BOARD_DIM; y++) {
            const Piece* p = &b->tiles[x][y].piece;
            if (p->type != PIECE_NONE
                && p->type != PIECE_COMMANDER
                && p->player == player) {
                count++;
            }
        }
    }
    return count;
}

bool board_has_commander(const Board* b, Player player) {
    for (int x = 0; x < BOARD_DIM; x++) {
        for (int y = 0; y < BOARD_DIM; y++) {
            const Piece* p = &b->tiles[x][y].piece;
            if (p->type == PIECE_COMMANDER && p->player == player) return true;
        }
    }
    return false;
}

WinState board_check_win(const Board* b) {
    bool black_cmd = board_has_commander(b, PLAYER_BLACK);
    bool white_cmd = board_has_commander(b, PLAYER_WHITE);

    if (!black_cmd && !white_cmd) return WIN_DRAW;
    if (!white_cmd) return WIN_BLACK;
    if (!black_cmd) return WIN_WHITE;

    int black_attackers = board_count_attack_pieces(b, PLAYER_BLACK);
    int white_attackers = board_count_attack_pieces(b, PLAYER_WHITE);
    /* If a player has only their commander left, they lose. */
    if (black_attackers == 0 && white_attackers == 0) return WIN_DRAW;
    if (black_attackers == 0) return WIN_WHITE;
    if (white_attackers == 0) return WIN_BLACK;

    return WIN_NONE;
}
