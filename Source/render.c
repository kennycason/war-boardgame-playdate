#include <stdio.h>
#include <string.h>
#include "render.h"
#include "piece.h"

static PlaydateAPI* pd          = NULL;
static LCDFont*     font_body   = NULL;
static LCDFont*     font_big    = NULL;
static int          frame_count = 0;

/* ---------------- Layout helpers ---------------- */

static int tile_screen_x(int bx) { return BOARD_ORIGIN_X + bx * TILE_SIZE; }
static int tile_screen_y(int by) { return BOARD_ORIGIN_Y + (BOARD_DIM - 1 - by) * TILE_SIZE; }

/* Vertical offset of the piece sprite for elevated render. */
static int elev_offset(RenderMode rm, int elev) {
    return (rm == RENDER_ELEVATED) ? (elev * 2) : 0;
}

static void draw_text(const char* s, int x, int y) {
    pd->graphics->drawText(s, strlen(s), kASCIIEncoding, x, y);
}

static void draw_text_centered_with(LCDFont* f, const char* s, int cx, int y) {
    int w = pd->graphics->getTextWidth(f, s, strlen(s), kASCIIEncoding, 0);
    pd->graphics->drawText(s, strlen(s), kASCIIEncoding, cx - w/2, y);
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

/* ---------------- Tile rendering ---------------- */

static void dot(int x, int y) {
    pd->graphics->fillRect(x, y, 2, 2, kColorBlack);
}

static void draw_flat_elev_marks(int sx, int sy, int elev) {
    if (elev <= 0) return;
    static const int dx[4] = {3, 7, 3, 7};
    static const int dy[4] = {3, 3, 7, 7};
    for (int i = 0; i < elev && i < 4; i++) {
        dot(sx + dx[i], sy + dy[i]);
    }
}

/* Draws the highlight overlay anchored at (sx, top_y) — the actual screen
 * position of the tile's top face (already accounts for elevation). */
static void draw_highlight(int sx, int top_y, TileHighlight h) {
    int sy = top_y;
    switch (h) {
    case HIGHLIGHT_SELECTED:
        pd->graphics->fillRect(sx,                 sy,                 TILE_SIZE, 2, kColorBlack);
        pd->graphics->fillRect(sx,                 sy + TILE_SIZE - 2, TILE_SIZE, 2, kColorBlack);
        pd->graphics->fillRect(sx,                 sy,                 2, TILE_SIZE, kColorBlack);
        pd->graphics->fillRect(sx + TILE_SIZE - 2, sy,                 2, TILE_SIZE, kColorBlack);
        break;
    case HIGHLIGHT_MOVE: {
        int cx = sx + TILE_SIZE / 2;
        int cy = sy + TILE_SIZE - 5;
        pd->graphics->fillTriangle(cx, cy - 2, cx - 2, cy, cx + 2, cy, kColorBlack);
        pd->graphics->fillTriangle(cx, cy + 2, cx - 2, cy, cx + 2, cy, kColorBlack);
    } break;
    case HIGHLIGHT_ATTACK:
        pd->graphics->fillRect(sx + 1,             sy + 1,             4, 1, kColorBlack);
        pd->graphics->fillRect(sx + 1,             sy + 1,             1, 4, kColorBlack);
        pd->graphics->fillRect(sx + TILE_SIZE - 5, sy + 1,             4, 1, kColorBlack);
        pd->graphics->fillRect(sx + TILE_SIZE - 2, sy + 1,             1, 4, kColorBlack);
        pd->graphics->fillRect(sx + 1,             sy + TILE_SIZE - 2, 4, 1, kColorBlack);
        pd->graphics->fillRect(sx + 1,             sy + TILE_SIZE - 5, 1, 4, kColorBlack);
        pd->graphics->fillRect(sx + TILE_SIZE - 5, sy + TILE_SIZE - 2, 4, 1, kColorBlack);
        pd->graphics->fillRect(sx + TILE_SIZE - 2, sy + TILE_SIZE - 5, 1, 4, kColorBlack);
        break;
    default: break;
    }
}

/* Clean tile top face (white interior + black border) at top_y.
 *
 * Border note: the body is filled at TILE_SIZE×TILE_SIZE, but the border is
 * drawn at (TILE_SIZE+1)×(TILE_SIZE+1). The extra pixel makes neighboring
 * tiles SHARE their adjoining border (one tile's right edge column = next
 * tile's left edge column), avoiding the double 2-pixel line that appears
 * when each tile draws its own 1-px border in its own cell. */
static void draw_tile_top(int sx, int top_y, TileHighlight h) {
    pd->graphics->fillRect(sx, top_y, TILE_SIZE, TILE_SIZE, kColorWhite);
    pd->graphics->drawRect(sx, top_y, TILE_SIZE + 1, TILE_SIZE + 1, kColorBlack);
    draw_highlight(sx, top_y, h);
}

/* South-face wall: terraced band that fills the vertical gap between the
 * elevated tile body and the natural ground row. One horizontal stripe per
 * elevation level so the user can literally count tiers. The wall's left and
 * right edges are drawn at columns shared with the next neighbor so vertical
 * lines don't double up. */
static void draw_south_wall(int sx, int sy_base, int elev) {
    if (elev <= 0) return;
    int elev_dy     = elev * 2;
    int wall_top    = sy_base + TILE_SIZE - elev_dy;
    int wall_bottom = sy_base + TILE_SIZE;

    /* Wipe to white so no piece/wall behind bleeds through. */
    pd->graphics->fillRect(sx, wall_top, TILE_SIZE, elev_dy, kColorWhite);

    /* Horizontal tier line per level. Each level is 2px tall; line marks the
     * bottom of each tier. */
    for (int t = 0; t < elev; t++) {
        int y = wall_top + t * 2 + 1;
        pd->graphics->drawLine(sx, y, sx + TILE_SIZE, y, 1, kColorBlack);
    }
    /* Left + right wall edges — extend to wall_bottom so they share that pixel
     * with the next row's top border. */
    pd->graphics->drawLine(sx,             wall_top, sx,             wall_bottom, 1, kColorBlack);
    pd->graphics->drawLine(sx + TILE_SIZE, wall_top, sx + TILE_SIZE, wall_bottom, 1, kColorBlack);
}

static void draw_tile_bg(int sx, int sy_base, int elev, TileHighlight h, RenderMode rm) {
    if (rm == RENDER_FLAT) {
        draw_tile_top(sx, sy_base, h);
        draw_flat_elev_marks(sx, sy_base, elev);
        return;
    }
    /* ELEVATED: wall first (below tile body), then the raised body on top. */
    int elev_dy = elev * 2;
    draw_south_wall(sx, sy_base, elev);
    draw_tile_top(sx, sy_base - elev_dy, h);
}

/* ---------------- Piece sprites ---------------- */

static void draw_infantry(int sx, int sy, Player pl) {
    int cx = sx + 8;
    ellipse_p(cx - 3, sy + 1, 6, 6, pl);
    rect_p(cx - 4, sy + 7, 8, 7, pl);
    if (pl == PLAYER_BLACK) {
        pd->graphics->fillRect(cx - 4, sy + 13, 3, 3, kColorBlack);
        pd->graphics->fillRect(cx + 1, sy + 13, 3, 3, kColorBlack);
    } else {
        pd->graphics->drawRect(cx - 4, sy + 13, 3, 3, kColorBlack);
        pd->graphics->drawRect(cx + 1, sy + 13, 3, 3, kColorBlack);
    }
}

static void draw_tank(int sx, int sy, Player pl) {
    rect_p(sx + 1, sy + 10, 14, 4, pl);
    rect_p(sx + 2, sy + 6, 12, 5, pl);
    rect_p(sx + 6, sy + 3, 6, 4, pl);
    rect_p(sx + 10, sy + 4, 5, 2, pl);
}

static void draw_sniper(int sx, int sy, Player pl) {
    ellipse_p(sx + 2, sy + 2, 12, 12, pl);
    LCDColor inner = (pl == PLAYER_BLACK) ? kColorWhite : kColorBlack;
    pd->graphics->drawLine(sx + 8, sy + 4, sx + 8, sy + 12, 1, inner);
    pd->graphics->drawLine(sx + 4, sy + 8, sx + 12, sy + 8, 1, inner);
    if (pl == PLAYER_BLACK) pd->graphics->fillRect(sx + 7, sy + 7, 2, 2, kColorWhite);
    else                    pd->graphics->fillRect(sx + 7, sy + 7, 2, 2, kColorBlack);
}

static void draw_artillery(int sx, int sy, Player pl) {
    ellipse_p(sx + 1,  sy + 10, 5, 5, pl);
    ellipse_p(sx + 10, sy + 10, 5, 5, pl);
    rect_p(sx + 3, sy + 7, 10, 4, pl);
    int lw = (pl == PLAYER_BLACK) ? 2 : 1;
    pd->graphics->drawLine(sx + 8, sy + 8, sx + 15, sy + 2, lw, kColorBlack);
}

static void draw_artillery_reloading(int sx, int sy) {
    pd->graphics->fillRect(sx + 12, sy + 13, 4, 4, kColorBlack);
    pd->graphics->fillRect(sx + 13, sy + 14, 2, 2, kColorWhite);
}

static void draw_missile(int sx, int sy, Player pl) {
    triangle_p(sx + 8, sy + 1, sx + 4, sy + 6, sx + 12, sy + 6, pl);
    rect_p(sx + 6, sy + 6, 5, 7, pl);
    triangle_p(sx + 6, sy + 13, sx + 2, sy + 15, sx + 6, sy + 15, pl);
    triangle_p(sx + 10, sy + 13, sx + 14, sy + 15, sx + 10, sy + 15, pl);
}

static void draw_air_defense(int sx, int sy, Player pl) {
    rect_p(sx + 2, sy + 11, 12, 5, pl);
    ellipse_p(sx + 3, sy + 4, 10, 8, pl);
    pd->graphics->drawLine(sx + 8, sy + 4, sx + 8, sy + 1, 1, kColorBlack);
    if (pl == PLAYER_BLACK) pd->graphics->fillRect(sx + 7, sy, 3, 2, kColorBlack);
    else                    pd->graphics->drawRect(sx + 7, sy, 3, 2, kColorBlack);
}

static void draw_bomber(int sx, int sy, Player pl) {
    rect_p(sx + 0, sy + 7, 16, 3, pl);
    rect_p(sx + 6, sy + 2, 4, 14, pl);
    triangle_p(sx + 6, sy + 2, sx + 10, sy + 2, sx + 8, sy, pl);
    rect_p(sx + 4, sy + 13, 8, 2, pl);
}

static void draw_commander(int sx, int sy, Player pl) {
    int cx = sx + 8, cy = sy + 8;
    rect_p(cx - 3, cy - 3, 6, 6, pl);
    triangle_p(cx, sy + 0,  cx - 3, cy - 3, cx + 3, cy - 3, pl);
    triangle_p(cx, sy + 15, cx - 3, cy + 3, cx + 3, cy + 3, pl);
    triangle_p(sx + 0,  cy, cx - 3, cy - 3, cx - 3, cy + 3, pl);
    triangle_p(sx + 15, cy, cx + 3, cy - 3, cx + 3, cy + 3, pl);
    if (pl == PLAYER_BLACK) pd->graphics->fillRect(cx - 1, cy - 1, 2, 2, kColorWhite);
    else                    pd->graphics->fillRect(cx - 1, cy - 1, 2, 2, kColorBlack);
}

static void draw_excavator(int sx, int sy, Player pl) {
    rect_p(sx + 3, sy + 11, 12, 4, pl);
    rect_p(sx + 5, sy + 6, 8, 5, pl);
    int lw = (pl == PLAYER_BLACK) ? 2 : 1;
    pd->graphics->drawLine(sx + 11, sy + 8, sx + 14, sy + 3, lw, kColorBlack);
    rect_p(sx + 13, sy + 1, 3, 3, pl);
}

static void draw_piece(int sx, int sy, const Piece* p) {
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
    int L = 5;
    LCDColor c = (LCDColor)kColorXOR;
    pd->graphics->fillRect(sx - 1, sy - 1, L, 2, c);
    pd->graphics->fillRect(sx - 1, sy - 1, 2, L, c);
    pd->graphics->fillRect(sx + TILE_SIZE - L + 1, sy - 1, L, 2, c);
    pd->graphics->fillRect(sx + TILE_SIZE - 1,     sy - 1, 2, L, c);
    pd->graphics->fillRect(sx - 1,                  sy + TILE_SIZE - 1, L, 2, c);
    pd->graphics->fillRect(sx - 1,                  sy + TILE_SIZE - L + 1, 2, L, c);
    pd->graphics->fillRect(sx + TILE_SIZE - L + 1,  sy + TILE_SIZE - 1, L, 2, c);
    pd->graphics->fillRect(sx + TILE_SIZE - 1,      sy + TILE_SIZE - L + 1, 2, L, c);
}

/* ---------------- Hourglass ---------------- */

static void draw_hourglass(int x, int y) {
    /* 12x14 hourglass — animated grain falling. */
    int phase = (frame_count / 6) % 3;
    /* Frame: top + bottom + middle pinch */
    pd->graphics->fillRect(x, y, 12, 2, kColorBlack);
    pd->graphics->fillRect(x, y + 12, 12, 2, kColorBlack);
    /* Triangular bulbs */
    triangle_p(x,     y + 2, x + 12, y + 2, x + 6, y + 7, PLAYER_BLACK);
    triangle_p(x,     y + 12, x + 12, y + 12, x + 6, y + 7, PLAYER_BLACK);
    /* sand pouring effect — white pixel mid-pinch */
    pd->graphics->fillRect(x + 5, y + 7 - phase, 2, 1, kColorWhite);
    pd->graphics->fillRect(x + 5, y + 7 + phase, 2, 1, kColorWhite);
}

/* ---------------- Board ---------------- */

static void draw_board(const Board* b, RenderMode rm, bool draw_cursor_flag,
                       int cursor_x, int cursor_y) {
    /* Draw row by row from back-row to front-row so elevated pieces in front
     * overlap the walls behind them. */
    for (int by = BOARD_DIM - 1; by >= 0; by--) {
        for (int x = 0; x < BOARD_DIM; x++) {
            int sx = tile_screen_x(x);
            int sy = tile_screen_y(by);
            const Tile* t = &b->tiles[x][by];
            draw_tile_bg(sx, sy, t->elevation, t->highlight, rm);
        }
        for (int x = 0; x < BOARD_DIM; x++) {
            int sx = tile_screen_x(x);
            int sy = tile_screen_y(by);
            const Tile* t = &b->tiles[x][by];
            if (t->piece.type != PIECE_NONE) {
                int dy = elev_offset(rm, t->elevation);
                draw_piece(sx, sy - dy, &t->piece);
            }
        }
    }
    if (draw_cursor_flag) {
        int dy = elev_offset(rm, b->tiles[cursor_x][cursor_y].elevation);
        draw_cursor(tile_screen_x(cursor_x), tile_screen_y(cursor_y) - dy);
    }
}

/* ---------------- Sidebar ---------------- */

static void draw_player_badge(int x, int y, Player pl) {
    if (pl == PLAYER_BLACK) pd->graphics->fillRect(x, y, 12, 12, kColorBlack);
    else                    pd->graphics->drawRect(x, y, 12, 12, kColorBlack);
}

static const char* difficulty_str(AIDifficulty d) {
    switch (d) {
    case AI_EASY:   return "EASY";
    case AI_HARD:   return "HARD";
    default:        return "NORMAL";
    }
}

static void draw_sidebar(GameState* g) {
    int x = SIDEBAR_X;
    int y = 4;
    char buf[64];

    pd->graphics->setFont(font_big);
    draw_text("WAR", x, y);
    y += 22;
    pd->graphics->drawLine(x, y, x + SIDEBAR_W, y, 1, kColorBlack);
    y += 4;

    pd->graphics->setFont(font_body);

    if (g->mode == GAME_MODE_AI_THINKING) {
        draw_hourglass(x, y);
        draw_text("THINKING", x + 18, y);
        y += 17;
    } else if (g->mode == GAME_MODE_GAME_OVER) {
        if (g->board.win_state == WIN_BLACK)      draw_text("BLACK WINS!", x, y);
        else if (g->board.win_state == WIN_WHITE) draw_text("WHITE WINS!", x, y);
        else                                      draw_text("DRAW",        x, y);
        y += 17;
        draw_text("A/B: title", x, y);
        y += 17;
    } else {
        draw_player_badge(x, y, g->board.current_player);
        draw_text(g->board.current_player == PLAYER_BLACK ? "BLACK" : "WHITE",
                  x + 16, y - 1);
        draw_text("TURN", x + 16 + 44, y - 1);
        y += 17;
    }

    snprintf(buf, sizeof(buf), "BLK %3d", g->board.black_score);
    draw_text(buf, x, y);
    snprintf(buf, sizeof(buf), "WHT %3d", g->board.white_score);
    draw_text(buf, x + 70, y);
    y += 18;

    pd->graphics->drawLine(x, y, x + SIDEBAR_W, y, 1, kColorBlack);
    y += 3;

    /* Cursor coords / review banner */
    if (g->review_offset < 0) {
        int target_turn = g->board.turn_count + g->review_offset;
        snprintf(buf, sizeof(buf), "PAST t%d", target_turn);
        draw_text(buf, x, y);
        y += 17;
        draw_text("hold B+crank", x, y);
        y += 17;
    } else {
        snprintf(buf, sizeof(buf), "@ %d,%d", g->cursor_x, g->cursor_y);
        draw_text(buf, x, y);
        y += 17;

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
    }

    pd->graphics->drawLine(x, y, x + SIDEBAR_W, y, 1, kColorBlack);
    y += 3;

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

/* ---------------- Title menu ---------------- */

static void draw_menu_row(const char* label, const char* value, int x, int y,
                          bool selected, bool has_arrows) {
    if (selected) {
        draw_text(">", x, y);
    }
    draw_text(label, x + 16, y);
    if (value) {
        if (has_arrows && selected) draw_text("<", x + 130, y);
        draw_text(value, x + 144, y);
        if (has_arrows && selected) draw_text(">", x + 200, y);
    }
}

static void draw_title(const GameState* g) {
    pd->graphics->clear(kColorWhite);

    pd->graphics->setFont(font_big);
    draw_text_centered_with(font_big, "W A R", LCD_COLUMNS / 2, 24);
    pd->graphics->drawLine(80, 50, LCD_COLUMNS - 80, 50, 1, kColorBlack);

    pd->graphics->setFont(font_body);

    int x = 80;
    int y = 70;
    draw_menu_row("Player vs Player", NULL, x, y, g->menu_row == 0, false);
    y += 24;
    draw_menu_row("Player vs AI",     NULL, x, y, g->menu_row == 1, false);
    y += 32;
    draw_menu_row("AI level",   difficulty_str(g->settings.ai_difficulty),
                  x, y, g->menu_row == 2, true);
    y += 22;
    draw_menu_row("Render",     g->settings.render == RENDER_FLAT ? "FLAT" : "ELEVATED",
                  x, y, g->menu_row == 3, true);
    y += 30;

    pd->graphics->drawLine(80, y, LCD_COLUMNS - 80, y, 1, kColorBlack);
    y += 6;
    draw_text_centered_with(font_body, "A : start    crank : AI level", LCD_COLUMNS / 2, y);
}

/* ---------------- Game-over overlay ---------------- */

static void draw_game_over_banner(GameState* g) {
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
    draw_text("A or B for title", bx + 28, by + 38);
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
        draw_title(g);
        return;
    }

    const Board* shown        = game_visible_board(g);
    bool         draw_cursor_ = (g->review_offset == 0
                                 && (g->mode == GAME_MODE_FREE
                                     || g->mode == GAME_MODE_PIECE_SELECTED));

    draw_board(shown, g->settings.render, draw_cursor_, g->cursor_x, g->cursor_y);
    draw_sidebar(g);

    if (g->mode == GAME_MODE_GAME_OVER && g->review_offset == 0) {
        draw_game_over_banner(g);
    }
}
