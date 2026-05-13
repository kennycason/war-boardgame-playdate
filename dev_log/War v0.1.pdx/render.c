#include <stdio.h>
#include <string.h>
#include "render.h"
#include "piece.h"

static PlaydateAPI* pd          = NULL;
static LCDFont*     font_body   = NULL;
static LCDFont*     font_big    = NULL;
static int          frame_count = 0;

/* ---------------- Helpers ---------------- */

static int tile_screen_x(int bx) { return BOARD_ORIGIN_X + bx * TILE_SIZE; }
static int tile_screen_y(int by) { return BOARD_ORIGIN_Y + (BOARD_DIM - 1 - by) * TILE_SIZE; }

static void draw_text(const char* s, int x, int y) {
    pd->graphics->drawText(s, strlen(s), kASCIIEncoding, x, y);
}

static void rect_p(int x, int y, int w, int h, Player pl) {
    if (pl == PLAYER_BLACK) pd->graphics->fillRect(x, y, w, h, kColorBlack);
    else                    pd->graphics->drawRect(x, y, w, h, kColorBlack);
}

static void ellipse_p(int x, int y, int w, int h, Player pl) {
    if (pl == PLAYER_BLACK) pd->graphics->fillEllipse(x, y, w, h, 0, 0, kColorBlack);
    else                    pd->graphics->drawEllipse(x, y, w, h, 1, 0, 0, kColorBlack);
}

static void triangle_p(int x1, int y1, int x2, int y2, int x3, int y3, Player pl) {
    if (pl == PLAYER_BLACK) {
        pd->graphics->fillTriangle(x1, y1, x2, y2, x3, y3, kColorBlack);
    } else {
        pd->graphics->drawLine(x1, y1, x2, y2, 1, kColorBlack);
        pd->graphics->drawLine(x2, y2, x3, y3, 1, kColorBlack);
        pd->graphics->drawLine(x3, y3, x1, y1, 1, kColorBlack);
    }
}

/* ---------------- Tile background & highlight ---------------- */

/* Draws a 2x2 pixel "dot" — visible elevation marker. */
static void dot(int x, int y) {
    pd->graphics->fillRect(x, y, 2, 2, kColorBlack);
}

static void draw_elevation_marks(int sx, int sy, int elev) {
    /* 1..4 small dots in the upper-left corner of the tile. */
    if (elev <= 0) return;
    static const int dx[4] = {3, 7, 3, 7};
    static const int dy[4] = {3, 3, 7, 7};
    for (int i = 0; i < elev && i < 4; i++) {
        dot(sx + dx[i], sy + dy[i]);
    }
}

static void draw_tile_bg(int sx, int sy, int elev, TileHighlight h) {
    /* base: white fill */
    pd->graphics->fillRect(sx, sy, TILE_SIZE, TILE_SIZE, kColorWhite);
    /* grid border */
    pd->graphics->drawRect(sx, sy, TILE_SIZE, TILE_SIZE, kColorBlack);
    /* elevation markers in upper-left */
    draw_elevation_marks(sx, sy, elev);

    switch (h) {
    case HIGHLIGHT_SELECTED: {
        /* Thick frame on all 4 edges */
        pd->graphics->fillRect(sx,                sy,                TILE_SIZE, 2, kColorBlack);
        pd->graphics->fillRect(sx,                sy + TILE_SIZE - 2, TILE_SIZE, 2, kColorBlack);
        pd->graphics->fillRect(sx,                sy,                2, TILE_SIZE, kColorBlack);
        pd->graphics->fillRect(sx + TILE_SIZE - 2, sy,                2, TILE_SIZE, kColorBlack);
    } break;
    case HIGHLIGHT_MOVE: {
        /* small filled diamond at center bottom (under any piece) */
        int cx = sx + TILE_SIZE / 2;
        int cy = sy + TILE_SIZE - 5;
        pd->graphics->fillTriangle(cx, cy - 2, cx - 2, cy, cx + 2, cy, kColorBlack);
        pd->graphics->fillTriangle(cx, cy + 2, cx - 2, cy, cx + 2, cy, kColorBlack);
    } break;
    case HIGHLIGHT_ATTACK: {
        /* 4 corner brackets — visible even with a piece in the middle */
        pd->graphics->fillRect(sx + 1,            sy + 1,            4, 1, kColorBlack);
        pd->graphics->fillRect(sx + 1,            sy + 1,            1, 4, kColorBlack);
        pd->graphics->fillRect(sx + TILE_SIZE - 5,sy + 1,            4, 1, kColorBlack);
        pd->graphics->fillRect(sx + TILE_SIZE - 2,sy + 1,            1, 4, kColorBlack);
        pd->graphics->fillRect(sx + 1,            sy + TILE_SIZE - 2,4, 1, kColorBlack);
        pd->graphics->fillRect(sx + 1,            sy + TILE_SIZE - 5,1, 4, kColorBlack);
        pd->graphics->fillRect(sx + TILE_SIZE - 5,sy + TILE_SIZE - 2,4, 1, kColorBlack);
        pd->graphics->fillRect(sx + TILE_SIZE - 2,sy + TILE_SIZE - 5,1, 4, kColorBlack);
    } break;
    default: break;
    }
}

/* ---------------- Piece sprites ---------------- */

/* Each draw function is given the top-left of a 16x16 sprite slot inside the tile. */

static void draw_infantry(int sx, int sy, Player pl) {
    int cx = sx + 8;
    /* head */
    ellipse_p(cx - 3, sy + 1, 6, 6, pl);
    /* body */
    rect_p(cx - 4, sy + 7, 8, 7, pl);
    /* legs (split) */
    if (pl == PLAYER_BLACK) {
        pd->graphics->fillRect(cx - 4, sy + 13, 3, 3, kColorBlack);
        pd->graphics->fillRect(cx + 1, sy + 13, 3, 3, kColorBlack);
    } else {
        pd->graphics->drawRect(cx - 4, sy + 13, 3, 3, kColorBlack);
        pd->graphics->drawRect(cx + 1, sy + 13, 3, 3, kColorBlack);
    }
}

static void draw_tank(int sx, int sy, Player pl) {
    /* tracks (rectangles at bottom) */
    rect_p(sx + 1, sy + 10, 14, 4, pl);
    /* hull */
    rect_p(sx + 2, sy + 6, 12, 5, pl);
    /* turret */
    rect_p(sx + 6, sy + 3, 6, 4, pl);
    /* gun barrel */
    rect_p(sx + 10, sy + 4, 5, 2, pl);
}

static void draw_sniper(int sx, int sy, Player pl) {
    /* Crosshair: outer circle + cross lines */
    ellipse_p(sx + 2, sy + 2, 12, 12, pl);
    /* inner cross — punch with white for BLACK to ensure visibility */
    LCDColor inner = (pl == PLAYER_BLACK) ? kColorWhite : kColorBlack;
    pd->graphics->drawLine(sx + 8, sy + 4, sx + 8, sy + 12, 1, inner);
    pd->graphics->drawLine(sx + 4, sy + 8, sx + 12, sy + 8, 1, inner);
    /* center dot */
    if (pl == PLAYER_BLACK) {
        pd->graphics->fillRect(sx + 7, sy + 7, 2, 2, kColorWhite);
    } else {
        pd->graphics->fillRect(sx + 7, sy + 7, 2, 2, kColorBlack);
    }
}

static void draw_artillery(int sx, int sy, Player pl) {
    /* wheels */
    ellipse_p(sx + 1,  sy + 10, 5, 5, pl);
    ellipse_p(sx + 10, sy + 10, 5, 5, pl);
    /* body — short rect */
    rect_p(sx + 3, sy + 7, 10, 4, pl);
    /* barrel — diagonal line going up-right */
    if (pl == PLAYER_BLACK) {
        pd->graphics->drawLine(sx + 8, sy + 8, sx + 15, sy + 2, 2, kColorBlack);
    } else {
        pd->graphics->drawLine(sx + 8, sy + 8, sx + 15, sy + 2, 1, kColorBlack);
    }
}

static void draw_artillery_reloading(int sx, int sy) {
    /* small "R" marker in lower-right corner */
    pd->graphics->fillRect(sx + 12, sy + 13, 4, 4, kColorBlack);
    pd->graphics->fillRect(sx + 13, sy + 14, 2, 2, kColorWhite);
}

static void draw_missile(int sx, int sy, Player pl) {
    /* triangle top — pointed up */
    triangle_p(sx + 8, sy + 1, sx + 4, sy + 6, sx + 12, sy + 6, pl);
    /* body */
    rect_p(sx + 6, sy + 6, 5, 7, pl);
    /* fins */
    triangle_p(sx + 6, sy + 13, sx + 2, sy + 15, sx + 6, sy + 15, pl);
    triangle_p(sx + 10, sy + 13, sx + 14, sy + 15, sx + 10, sy + 15, pl);
}

static void draw_air_defense(int sx, int sy, Player pl) {
    /* base box */
    rect_p(sx + 2, sy + 11, 12, 5, pl);
    /* dish (half ellipse) - drawn as full ellipse, base box covers bottom */
    ellipse_p(sx + 3, sy + 4, 10, 8, pl);
    /* antenna stick */
    pd->graphics->drawLine(sx + 8, sy + 4, sx + 8, sy + 1, 1, kColorBlack);
    if (pl == PLAYER_BLACK) {
        pd->graphics->fillRect(sx + 7, sy, 3, 2, kColorBlack);
    } else {
        pd->graphics->drawRect(sx + 7, sy, 3, 2, kColorBlack);
    }
}

static void draw_bomber(int sx, int sy, Player pl) {
    /* main wings — long horizontal bar */
    rect_p(sx + 0, sy + 7, 16, 3, pl);
    /* fuselage — vertical bar */
    rect_p(sx + 6, sy + 2, 4, 14, pl);
    /* nose triangle */
    triangle_p(sx + 6, sy + 2, sx + 10, sy + 2, sx + 8, sy, pl);
    /* tail fins */
    rect_p(sx + 4, sy + 13, 8, 2, pl);
}

static void draw_commander(int sx, int sy, Player pl) {
    /* 5-pointed star approximation — center diamond + 4 spike triangles */
    int cx = sx + 8, cy = sy + 8;
    /* center */
    rect_p(cx - 3, cy - 3, 6, 6, pl);
    /* spikes */
    triangle_p(cx, sy + 0,  cx - 3, cy - 3, cx + 3, cy - 3, pl); /* top */
    triangle_p(cx, sy + 15, cx - 3, cy + 3, cx + 3, cy + 3, pl); /* bottom */
    triangle_p(sx + 0,  cy, cx - 3, cy - 3, cx - 3, cy + 3, pl); /* left */
    triangle_p(sx + 15, cy, cx + 3, cy - 3, cx + 3, cy + 3, pl); /* right */
    /* inner mark — small white dot for BLACK, black dot for WHITE */
    if (pl == PLAYER_BLACK) {
        pd->graphics->fillRect(cx - 1, cy - 1, 2, 2, kColorWhite);
    } else {
        pd->graphics->fillRect(cx - 1, cy - 1, 2, 2, kColorBlack);
    }
}

static void draw_excavator(int sx, int sy, Player pl) {
    /* tracks */
    rect_p(sx + 3, sy + 11, 12, 4, pl);
    /* cab */
    rect_p(sx + 5, sy + 6, 8, 5, pl);
    /* arm */
    if (pl == PLAYER_BLACK) {
        pd->graphics->drawLine(sx + 11, sy + 8, sx + 14, sy + 3, 2, kColorBlack);
    } else {
        pd->graphics->drawLine(sx + 11, sy + 8, sx + 14, sy + 3, 1, kColorBlack);
    }
    /* bucket */
    rect_p(sx + 13, sy + 1, 3, 3, pl);
}

static void draw_piece(int sx, int sy, const Piece* p) {
    /* sprite area: (sx+2, sy+2) → (sx+18, sy+18), i.e. 16x16 inside 20x20 tile */
    int psx = sx + 2;
    int psy = sy + 2;
    switch (p->type) {
    case PIECE_INFANTRY:    draw_infantry(psx, psy, p->player); break;
    case PIECE_TANK:        draw_tank(psx, psy, p->player); break;
    case PIECE_SNIPER:      draw_sniper(psx, psy, p->player); break;
    case PIECE_ARTILLERY:
        draw_artillery(psx, psy, p->player);
        if (p->reloading) draw_artillery_reloading(sx, sy);
        break;
    case PIECE_MISSILE:     draw_missile(psx, psy, p->player); break;
    case PIECE_AIR_DEFENSE: draw_air_defense(psx, psy, p->player); break;
    case PIECE_BOMBER:      draw_bomber(psx, psy, p->player); break;
    case PIECE_COMMANDER:   draw_commander(psx, psy, p->player); break;
    case PIECE_EXCAVATOR:   draw_excavator(psx, psy, p->player); break;
    default: break;
    }
}

/* ---------------- Cursor ---------------- */

static void draw_cursor(int sx, int sy) {
    /* Four corner brackets, drawn with XOR so always visible. */
    int L = 5; /* arm length */
    LCDColor c = (LCDColor)kColorXOR;
    /* top-left */
    pd->graphics->fillRect(sx - 1, sy - 1, L, 2, c);
    pd->graphics->fillRect(sx - 1, sy - 1, 2, L, c);
    /* top-right */
    pd->graphics->fillRect(sx + TILE_SIZE - L + 1, sy - 1, L, 2, c);
    pd->graphics->fillRect(sx + TILE_SIZE - 1,     sy - 1, 2, L, c);
    /* bottom-left */
    pd->graphics->fillRect(sx - 1,                  sy + TILE_SIZE - 1, L, 2, c);
    pd->graphics->fillRect(sx - 1,                  sy + TILE_SIZE - L + 1, 2, L, c);
    /* bottom-right */
    pd->graphics->fillRect(sx + TILE_SIZE - L + 1,  sy + TILE_SIZE - 1, L, 2, c);
    pd->graphics->fillRect(sx + TILE_SIZE - 1,      sy + TILE_SIZE - L + 1, 2, L, c);
}

/* ---------------- Board ---------------- */

static void draw_board(GameState* g) {
    for (int x = 0; x < BOARD_DIM; x++) {
        for (int y = 0; y < BOARD_DIM; y++) {
            int sx = tile_screen_x(x);
            int sy = tile_screen_y(y);
            const Tile* t = &g->board.tiles[x][y];
            draw_tile_bg(sx, sy, t->elevation, t->highlight);
            if (t->piece.type != PIECE_NONE) {
                draw_piece(sx, sy, &t->piece);
            }
        }
    }
    /* Cursor on top, but only while playing. */
    if (g->mode == GAME_MODE_FREE || g->mode == GAME_MODE_PIECE_SELECTED) {
        draw_cursor(tile_screen_x(g->cursor_x), tile_screen_y(g->cursor_y));
    }
}

/* ---------------- Sidebar ---------------- */

static void draw_player_badge(int x, int y, Player pl) {
    if (pl == PLAYER_BLACK) pd->graphics->fillRect(x, y, 12, 12, kColorBlack);
    else                    pd->graphics->drawRect(x, y, 12, 12, kColorBlack);
}

static void draw_sidebar(GameState* g) {
    int x  = SIDEBAR_X;
    int y  = 4;
    char buf[64];

    /* Title bar */
    pd->graphics->setFont(font_big);
    draw_text("WAR", x, y);
    y += 22;

    /* Divider */
    pd->graphics->drawLine(x, y, x + SIDEBAR_W, y, 1, kColorBlack);
    y += 4;

    pd->graphics->setFont(font_body);

    /* Current turn */
    if (g->mode != GAME_MODE_GAME_OVER) {
        draw_player_badge(x, y, g->board.current_player);
        draw_text(g->board.current_player == PLAYER_BLACK ? "BLACK" : "WHITE",
                  x + 16, y - 1);
        draw_text("TURN", x + 16 + 44, y - 1);
        y += 17;
    } else {
        if (g->board.win_state == WIN_BLACK)      draw_text("BLACK WINS!", x, y);
        else if (g->board.win_state == WIN_WHITE) draw_text("WHITE WINS!", x, y);
        else                                      draw_text("DRAW",        x, y);
        y += 17;
        draw_text("press A/B", x, y);
        y += 17;
    }

    /* Scores */
    snprintf(buf, sizeof(buf), "BLK %3d", g->board.black_score);
    draw_text(buf, x, y);
    snprintf(buf, sizeof(buf), "WHT %3d", g->board.white_score);
    draw_text(buf, x + 70, y);
    y += 18;

    /* Divider */
    pd->graphics->drawLine(x, y, x + SIDEBAR_W, y, 1, kColorBlack);
    y += 3;

    /* Cursor coords */
    snprintf(buf, sizeof(buf), "@ %d,%d", g->cursor_x, g->cursor_y);
    draw_text(buf, x, y);
    y += 17;

    /* Selected piece */
    if (g->mode == GAME_MODE_PIECE_SELECTED) {
        const Piece* sp = &g->board.tiles[g->selected_x][g->selected_y].piece;
        snprintf(buf, sizeof(buf), "%s", piece_name(sp->type));
        draw_text(buf, x, y);
        y += 17;
        if (sp->type == PIECE_ARTILLERY && sp->reloading) {
            draw_text("[RELOAD]", x, y);
            y += 17;
        }
    } else {
        draw_text("- - -", x, y);
        y += 17;
    }

    /* Divider */
    pd->graphics->drawLine(x, y, x + SIDEBAR_W, y, 1, kColorBlack);
    y += 3;

    /* Last move */
    draw_text("LAST", x, y);
    y += 16;
    if (g->has_last_move) {
        const Move* m = &g->last_move;
        char tag = (m->move_type == MOVE_TYPE_ATTACK) ? '*' : ' ';
        snprintf(buf, sizeof(buf), "%c%s %s",
                 tag,
                 m->player == PLAYER_BLACK ? "B" : "W",
                 piece_short_name(m->piece_type));
        draw_text(buf, x, y);
        y += 16;
        snprintf(buf, sizeof(buf), "%d,%d>%d,%d",
                 m->from_x, m->from_y, m->to_x, m->to_y);
        draw_text(buf, x, y);
        y += 16;
        if (m->intercepted) {
            draw_text("INTERCEPT!", x, y);
            y += 16;
        }
    }
}

/* ---------------- Title overlay ---------------- */

static void draw_title(void) {
    pd->graphics->clear(kColorWhite);
    pd->graphics->setFont(font_big);
    draw_text("WAR", 170, 60);
    pd->graphics->setFont(font_body);
    draw_text("a chess-like strategy game", 100, 100);
    draw_text("D-pad : move cursor",  130, 130);
    draw_text("    A : select / confirm", 130, 148);
    draw_text("    B : cancel",       130, 166);
    draw_text("press A to begin", 140, 210);
}

/* ---------------- Game-over banner ---------------- */

static void draw_game_over_banner(GameState* g) {
    /* Centered banner over the board */
    int bw = 200, bh = 60;
    int bx = (BOARD_ORIGIN_X + BOARD_PX_SIZE / 2) - bw / 2;
    int by = (BOARD_ORIGIN_Y + BOARD_PX_SIZE / 2) - bh / 2;
    pd->graphics->fillRect(bx,     by,     bw,     bh,     kColorWhite);
    pd->graphics->drawRect(bx,     by,     bw,     bh,     kColorBlack);
    pd->graphics->drawRect(bx - 2, by - 2, bw + 4, bh + 4, kColorBlack);

    pd->graphics->setFont(font_big);
    const char* msg;
    if (g->board.win_state == WIN_BLACK)      msg = "BLACK WINS";
    else if (g->board.win_state == WIN_WHITE) msg = "WHITE WINS";
    else                                      msg = "DRAW";
    draw_text(msg, bx + 30, by + 10);

    pd->graphics->setFont(font_body);
    draw_text("press A to restart", bx + 20, by + 38);
}

/* ---------------- Entry points ---------------- */

void render_init(PlaydateAPI* api) {
    pd = api;
    const char* err = NULL;
    font_body = pd->graphics->loadFont("/System/Fonts/Asheville-Sans-14-Light.pft", &err);
    if (!font_body) {
        font_body = pd->graphics->loadFont("/System/Fonts/Roobert-11-Medium.pft", &err);
    }
    font_big = pd->graphics->loadFont("/System/Fonts/Asheville-Sans-14-Bold.pft", &err);
    if (!font_big) font_big = font_body;
    if (font_body) pd->graphics->setFont(font_body);
}

void render_frame(GameState* g) {
    frame_count++;
    pd->graphics->clear(kColorWhite);

    if (g->mode == GAME_MODE_TITLE) {
        draw_title();
        return;
    }

    draw_board(g);
    draw_sidebar(g);

    if (g->mode == GAME_MODE_GAME_OVER) {
        draw_game_over_banner(g);
    }
}
