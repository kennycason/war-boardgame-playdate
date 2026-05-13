#ifndef WAR_PIECE_H
#define WAR_PIECE_H

#include <stdbool.h>
#include "types.h"

typedef struct {
    PieceType type;
    Player    player;
    int       x;
    int       y;
    /* artillery state */
    bool      reloading;
    int       last_attack_turn;
} Piece;

int         piece_score(PieceType type);
const char* piece_name(PieceType type);       /* "INFANTRY" */
const char* piece_short_name(PieceType type); /* "INF" */
char        piece_letter(PieceType type);     /* 'I' */

#endif
