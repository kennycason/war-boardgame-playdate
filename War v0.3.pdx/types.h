#ifndef WAR_TYPES_H
#define WAR_TYPES_H

#define BOARD_DIM            11
#define MAX_MOVES_PER_PIECE  96
#define ARTILLERY_RELOAD_TURNS 3
#define MIN_ELEVATION         0
#define MAX_ELEVATION         4
#define COMMANDER_SCORE       100

typedef enum {
    PLAYER_NONE  = 0,
    PLAYER_BLACK = 1,
    PLAYER_WHITE = 2,
} Player;

typedef enum {
    PIECE_NONE = 0,
    PIECE_INFANTRY,
    PIECE_TANK,
    PIECE_SNIPER,
    PIECE_ARTILLERY,
    PIECE_MISSILE,
    PIECE_AIR_DEFENSE,
    PIECE_BOMBER,
    PIECE_COMMANDER,
    PIECE_EXCAVATOR,
    PIECE_TYPE_COUNT,
} PieceType;

typedef enum {
    MOVE_TYPE_MOVE,
    MOVE_TYPE_ATTACK,
} MoveType;

typedef enum {
    HIGHLIGHT_NONE,
    HIGHLIGHT_MOVE,
    HIGHLIGHT_ATTACK,
    HIGHLIGHT_SELECTED,
} TileHighlight;

typedef enum {
    WIN_NONE,
    WIN_BLACK,
    WIN_WHITE,
    WIN_DRAW,
} WinState;

typedef enum {
    GAME_MODE_FREE,           /* cursor floats, no piece selected */
    GAME_MODE_PIECE_SELECTED, /* a piece is selected, cursor picks destination */
    GAME_MODE_AI_THINKING,    /* AI is computing its move */
    GAME_MODE_GAME_OVER,
    GAME_MODE_TITLE,
} GameMode;

typedef enum {
    MATCH_PVP = 0,
    MATCH_PVA = 1,
} MatchType;

#define AI_LEVEL_MIN 1
#define AI_LEVEL_MAX 9

typedef enum {
    RENDER_FLAT     = 0,
    RENDER_ELEVATED = 1,
} RenderMode;

typedef struct {
    MatchType  match;
    int        ai_level;     /* 1..9 — see ai.c for level → depth/randomness map */
    Player     ai_player;    /* which color the AI controls in PvA */
    RenderMode render;
} Settings;

#define HISTORY_CAP 200

#endif
