#include <stdlib.h>
#include <string.h>
#include "move.h"

static int abs_i(int v) { return v < 0 ? -v : v; }

/* Ray casting config */
typedef struct {
    int  min_dist;
    int  max_dist;
    bool can_attack;
    bool required_attack; /* if true, only ATTACK moves are emitted (no plain moves) */
    bool can_pass_own;
    bool can_pass_enemy;
    bool ignore_height;
} RayCfg;

static int emit(Move* out, int idx, int max,
                Player pl, PieceType pt, MoveType mt,
                int fx, int fy, int tx, int ty,
                int score, PieceType dt, Player dp) {
    if (idx >= max) return idx;
    Move* m = &out[idx];
    m->player           = pl;
    m->piece_type       = pt;
    m->move_type        = mt;
    m->from_x           = fx;
    m->from_y           = fy;
    m->to_x             = tx;
    m->to_y             = ty;
    m->score            = score;
    m->destroyed_type   = dt;
    m->destroyed_player = dp;
    m->elevation_delta  = 0;
    m->intercepted      = false;
    m->intercept_x      = -1;
    m->intercept_y      = -1;
    return idx + 1;
}

static int cast_ray(Board* b, int x, int y, int dx, int dy,
                    Player player, PieceType pt,
                    RayCfg cfg, Move* out, int idx, int max) {
    for (int i = cfg.min_dist; i <= cfg.max_dist; i++) {
        int tx = x + dx * i;
        int ty = y + dy * i;
        if (!board_in_bounds(tx, ty)) break;

        if (!cfg.ignore_height) {
            int px = x + dx * (i - 1);
            int py = y + dy * (i - 1);
            int delta = abs_i(b->tiles[tx][ty].elevation - b->tiles[px][py].elevation);
            if (delta > 1) break;
        }

        Piece* tp = &b->tiles[tx][ty].piece;
        if (tp->type == PIECE_NONE) {
            if (!cfg.required_attack) {
                idx = emit(out, idx, max, player, pt, MOVE_TYPE_MOVE,
                           x, y, tx, ty, 0, PIECE_NONE, PLAYER_NONE);
            }
        } else {
            bool is_enemy = (cfg.can_attack && tp->player != player);
            if (is_enemy) {
                idx = emit(out, idx, max, player, pt, MOVE_TYPE_ATTACK,
                           x, y, tx, ty, piece_score(tp->type),
                           tp->type, tp->player);
                if (!cfg.can_pass_enemy) break;
            } else {
                if (!cfg.can_pass_own) break;
            }
        }
    }
    return idx;
}

static int gen_hv(Board* b, int x, int y, Player p, PieceType pt,
                  RayCfg cfg, Move* out, int idx, int max) {
    idx = cast_ray(b, x, y, -1,  0, p, pt, cfg, out, idx, max);
    idx = cast_ray(b, x, y, +1,  0, p, pt, cfg, out, idx, max);
    idx = cast_ray(b, x, y,  0, -1, p, pt, cfg, out, idx, max);
    idx = cast_ray(b, x, y,  0, +1, p, pt, cfg, out, idx, max);
    return idx;
}

static int gen_diag(Board* b, int x, int y, Player p, PieceType pt,
                    RayCfg cfg, Move* out, int idx, int max) {
    idx = cast_ray(b, x, y, -1, -1, p, pt, cfg, out, idx, max);
    idx = cast_ray(b, x, y, +1, -1, p, pt, cfg, out, idx, max);
    idx = cast_ray(b, x, y, -1, +1, p, pt, cfg, out, idx, max);
    idx = cast_ray(b, x, y, +1, +1, p, pt, cfg, out, idx, max);
    return idx;
}

/* ---- Per-piece move generation ---- */

static int gen_infantry(Board* b, Piece* p, Move* out, int max) {
    int idx = 0;
    RayCfg hv = { 1, 1, false, false, false, false, false };
    idx = gen_hv(b, p->x, p->y, p->player, p->type, hv, out, idx, max);
    RayCfg dg = { 1, 1, true,  true,  false, false, false };
    idx = gen_diag(b, p->x, p->y, p->player, p->type, dg, out, idx, max);
    return idx;
}

static int gen_tank(Board* b, Piece* p, Move* out, int max) {
    RayCfg hv = { 1, 2, true, false, false, false, false };
    return gen_hv(b, p->x, p->y, p->player, p->type, hv, out, 0, max);
}

static int gen_sniper(Board* b, Piece* p, Move* out, int max) {
    RayCfg dg = { 1, 2, true, false, false, false, false };
    return gen_diag(b, p->x, p->y, p->player, p->type, dg, out, 0, max);
}

static int gen_commander(Board* b, Piece* p, Move* out, int max) {
    RayCfg c = { 1, 1, true, false, false, false, false };
    int idx = gen_hv(b, p->x, p->y, p->player, p->type, c, out, 0, max);
    idx = gen_diag(b, p->x, p->y, p->player, p->type, c, out, idx, max);
    return idx;
}

static int gen_artillery(Board* b, Piece* p, Move* out, int max) {
    if (p->reloading && (b->turn_count - p->last_attack_turn) > ARTILLERY_RELOAD_TURNS) {
        p->reloading = false;
    }
    int idx = 0;
    RayCfg move = { 1, 1, false, false, false, false, false };
    idx = gen_hv(b, p->x, p->y, p->player, p->type, move, out, idx, max);
    idx = gen_diag(b, p->x, p->y, p->player, p->type, move, out, idx, max);
    if (!p->reloading) {
        RayCfg atk = { 2, 3, true, true, true, true, true };
        idx = gen_hv(b, p->x, p->y, p->player, p->type, atk, out, idx, max);
    }
    return idx;
}

static int gen_missile(Board* b, Piece* p, Move* out, int max) {
    int idx = 0;
    RayCfg atk = { 2, 5, true, true, true, true, true };
    idx = gen_diag(b, p->x, p->y, p->player, p->type, atk, out, idx, max);
    RayCfg move = { 1, 1, false, false, false, false, false };
    idx = gen_hv(b, p->x, p->y, p->player, p->type, move, out, idx, max);
    idx = gen_diag(b, p->x, p->y, p->player, p->type, move, out, idx, max);
    return idx;
}

static int gen_air_defense(Board* b, Piece* p, Move* out, int max) {
    RayCfg c = { 1, 1, false, false, false, false, false };
    int idx = gen_hv(b, p->x, p->y, p->player, p->type, c, out, 0, max);
    idx = gen_diag(b, p->x, p->y, p->player, p->type, c, out, idx, max);
    return idx;
}

static int gen_bomber(Board* b, Piece* p, Move* out, int max) {
    RayCfg c = { 1, 4, true, false, true, false, true };
    return gen_hv(b, p->x, p->y, p->player, p->type, c, out, 0, max);
}

static int gen_excavator(Board* b, Piece* p, Move* out, int max) {
    int idx = 0;
    RayCfg c = { 1, 1, false, false, false, false, false };
    idx = gen_hv(b, p->x, p->y, p->player, p->type, c, out, idx, max);
    idx = gen_diag(b, p->x, p->y, p->player, p->type, c, out, idx, max);

    int original = idx;
    for (int i = 0; i < original; i++) {
        Move* m = &out[i];
        if (m->move_type != MOVE_TYPE_MOVE) continue;
        Tile* t = &b->tiles[m->to_x][m->to_y];
        if (t->piece.type != PIECE_NONE) continue;

        if (t->elevation > MIN_ELEVATION && idx < max) {
            Move* em = &out[idx++];
            *em = *m;
            em->move_type       = MOVE_TYPE_ATTACK;
            em->score           = 0;
            em->elevation_delta = -1;
        }
        if (t->elevation < MAX_ELEVATION && idx < max) {
            Move* em = &out[idx++];
            *em = *m;
            em->move_type       = MOVE_TYPE_ATTACK;
            em->score           = 0;
            em->elevation_delta = +1;
        }
    }
    return idx;
}

int generate_moves(Board* b, int x, int y, Move* out, int max) {
    if (!board_in_bounds(x, y)) return 0;
    Piece* p = &b->tiles[x][y].piece;
    if (p->type == PIECE_NONE) return 0;

    switch (p->type) {
    case PIECE_INFANTRY:    return gen_infantry(b, p, out, max);
    case PIECE_TANK:        return gen_tank(b, p, out, max);
    case PIECE_SNIPER:      return gen_sniper(b, p, out, max);
    case PIECE_ARTILLERY:   return gen_artillery(b, p, out, max);
    case PIECE_MISSILE:     return gen_missile(b, p, out, max);
    case PIECE_AIR_DEFENSE: return gen_air_defense(b, p, out, max);
    case PIECE_BOMBER:      return gen_bomber(b, p, out, max);
    case PIECE_COMMANDER:   return gen_commander(b, p, out, max);
    case PIECE_EXCAVATOR:   return gen_excavator(b, p, out, max);
    default: return 0;
    }
}

/* ---- Air Defense detection (8-neighbor enemy AD) ---- */
static bool find_air_defense(const Board* b, Player attacker, int tx, int ty,
                             int* out_x, int* out_y) {
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            int nx = tx + dx;
            int ny = ty + dy;
            if (!board_in_bounds(nx, ny)) continue;
            const Piece* p = &b->tiles[nx][ny].piece;
            if (p->type == PIECE_AIR_DEFENSE && p->player != attacker) {
                *out_x = nx;
                *out_y = ny;
                return true;
            }
        }
    }
    return false;
}

/* ---- Apply ---- */

static void add_score(Board* b, Player p, int score) {
    if (p == PLAYER_BLACK) b->black_score += score;
    else if (p == PLAYER_WHITE) b->white_score += score;
}

static void relocate(Board* b, int fx, int fy, int tx, int ty) {
    Piece moving = b->tiles[fx][fy].piece;
    b->tiles[fx][fy].piece.type = PIECE_NONE;
    moving.x = tx;
    moving.y = ty;
    b->tiles[tx][ty].piece = moving;
}

static void switch_turn(Board* b) {
    b->turn_count++;
    b->current_player = (b->current_player == PLAYER_BLACK) ? PLAYER_WHITE : PLAYER_BLACK;
}

void apply_move(Board* b, Move* mv) {
    Piece src = b->tiles[mv->from_x][mv->from_y].piece;
    Player srcPlayer = src.player;
    PieceType srcType = src.type;

    mv->intercepted = false;
    mv->intercept_x = -1;
    mv->intercept_y = -1;

    if (mv->move_type == MOVE_TYPE_MOVE) {
        /* Bombers can be intercepted on movement too. */
        if (srcType == PIECE_BOMBER) {
            int adx, ady;
            if (find_air_defense(b, srcPlayer, mv->to_x, mv->to_y, &adx, &ady)) {
                b->tiles[mv->from_x][mv->from_y].piece.type = PIECE_NONE;
                b->tiles[adx][ady].piece.type = PIECE_NONE;
                mv->intercepted = true;
                mv->intercept_x = adx;
                mv->intercept_y = ady;
                goto end;
            }
        }
        relocate(b, mv->from_x, mv->from_y, mv->to_x, mv->to_y);
        if (mv->elevation_delta != 0) {
            b->tiles[mv->to_x][mv->to_y].elevation += mv->elevation_delta;
        }
    } else { /* ATTACK */
        if (srcType == PIECE_ARTILLERY) {
            /* Stays in place, target destroyed, sets reload. */
            b->tiles[mv->to_x][mv->to_y].piece.type = PIECE_NONE;
            b->tiles[mv->from_x][mv->from_y].piece.reloading = true;
            b->tiles[mv->from_x][mv->from_y].piece.last_attack_turn = b->turn_count;
            add_score(b, srcPlayer, mv->score);
        } else if (srcType == PIECE_MISSILE) {
            /* Self-destructs; target dies unless intercepted. */
            int adx, ady;
            bool intercepted = find_air_defense(b, srcPlayer, mv->to_x, mv->to_y, &adx, &ady);
            b->tiles[mv->from_x][mv->from_y].piece.type = PIECE_NONE;
            if (intercepted) {
                b->tiles[adx][ady].piece.type = PIECE_NONE;
                mv->intercepted = true;
                mv->intercept_x = adx;
                mv->intercept_y = ady;
                /* No score when intercepted. */
            } else {
                b->tiles[mv->to_x][mv->to_y].piece.type = PIECE_NONE;
                add_score(b, srcPlayer, mv->score);
            }
        } else if (srcType == PIECE_BOMBER) {
            int adx, ady;
            bool intercepted = find_air_defense(b, srcPlayer, mv->to_x, mv->to_y, &adx, &ady);
            if (intercepted) {
                b->tiles[mv->from_x][mv->from_y].piece.type = PIECE_NONE;
                b->tiles[adx][ady].piece.type = PIECE_NONE;
                mv->intercepted = true;
                mv->intercept_x = adx;
                mv->intercept_y = ady;
                /* target survives, no score */
            } else {
                b->tiles[mv->to_x][mv->to_y].piece.type = PIECE_NONE;
                relocate(b, mv->from_x, mv->from_y, mv->to_x, mv->to_y);
                add_score(b, srcPlayer, mv->score);
            }
        } else if (srcType == PIECE_EXCAVATOR && mv->elevation_delta != 0) {
            relocate(b, mv->from_x, mv->from_y, mv->to_x, mv->to_y);
            b->tiles[mv->to_x][mv->to_y].elevation += mv->elevation_delta;
        } else {
            /* Standard melee attack: relocate + destroy target. */
            b->tiles[mv->to_x][mv->to_y].piece.type = PIECE_NONE;
            relocate(b, mv->from_x, mv->from_y, mv->to_x, mv->to_y);
            add_score(b, srcPlayer, mv->score);
        }
    }

end:
    switch_turn(b);
    b->win_state = board_check_win(b);
}

bool find_move(Board* b, int from_x, int from_y, int to_x, int to_y,
               int elevation_delta, Move* out) {
    Move moves[MAX_MOVES_PER_PIECE];
    int count = generate_moves(b, from_x, from_y, moves, MAX_MOVES_PER_PIECE);
    for (int i = 0; i < count; i++) {
        if (moves[i].to_x == to_x
            && moves[i].to_y == to_y
            && moves[i].elevation_delta == elevation_delta) {
            *out = moves[i];
            return true;
        }
    }
    return false;
}
