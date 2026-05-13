#ifndef WAR_BOARD_H
#define WAR_BOARD_H

#include <stdbool.h>
#include "types.h"
#include "piece.h"

typedef struct {
    int           elevation;
    Piece         piece;       /* type == PIECE_NONE means empty */
    TileHighlight highlight;
} Tile;

typedef struct {
    Tile     tiles[BOARD_DIM][BOARD_DIM];
    Player   current_player;
    int      turn_count;
    int      black_score;
    int      white_score;
    WinState win_state;
} Board;

void board_init(Board* b);
void board_reset(Board* b, MapLayout map);
void board_clear_highlights(Board* b);
void board_place_piece(Board* b, PieceType type, Player player, int x, int y);
void board_setup_pieces(Board* b);
void board_setup_terrain(Board* b, MapLayout map);

const char* board_map_name(MapLayout map);

bool     board_in_bounds(int x, int y);
WinState board_check_win(const Board* b);
int      board_count_attack_pieces(const Board* b, Player player);
bool     board_has_commander(const Board* b, Player player);

#endif
