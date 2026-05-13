#include "input.h"

/* On the title screen the crank cycles AI difficulty. */
static void handle_title(PlaydateAPI* pd, GameState* g, PDButtons pushed, float crank) {
    (void)pd;
    if (pushed & kButtonUp)    game_title_nav(g, -1);
    if (pushed & kButtonDown)  game_title_nav(g, +1);
    if (pushed & kButtonLeft)  game_title_change(g, -1);
    if (pushed & kButtonRight) game_title_change(g, +1);

    /* Crank changes difficulty regardless of row — 30° per click. */
    static float accum = 0.0f;
    accum += crank;
    while (accum >= 30.0f) {
        AIDifficulty d = g->settings.ai_difficulty;
        d = (d == AI_HARD) ? AI_HARD : (AIDifficulty)(d + 1);
        g->settings.ai_difficulty = d;
        accum -= 30.0f;
    }
    while (accum <= -30.0f) {
        AIDifficulty d = g->settings.ai_difficulty;
        d = (d == AI_EASY) ? AI_EASY : (AIDifficulty)(d - 1);
        g->settings.ai_difficulty = d;
        accum += 30.0f;
    }

    if (pushed & kButtonA) game_title_select(g);
}

/* B-held + crank time-travel. Returns true if review is engaged (consume input).
 * A bare B-press without cranking does NOT engage — it falls through so the
 * usual B action (cancel selection / game-over→title) still works. */
static bool handle_time_travel(GameState* g, PDButtons current, float crank) {
    bool b_now = (current & kButtonB) != 0;

    if (!b_now) {
        if (g->review_offset != 0) {
            g->review_offset      = 0;
            g->review_crank_accum = 0.0f;
        }
        g->b_held = false;
        return false;
    }

    g->b_held = true;
    g->review_crank_accum += crank;

    bool stepped = false;
    while (g->review_crank_accum >= 25.0f) {
        g->review_offset += 1;
        g->review_crank_accum -= 25.0f;
        stepped = true;
    }
    while (g->review_crank_accum <= -25.0f) {
        g->review_offset -= 1;
        g->review_crank_accum += 25.0f;
        stepped = true;
    }

    /* Engage only once review is in motion. Otherwise B-press should still
     * deliver its normal cancel/dismiss action. */
    if (!stepped && g->review_offset == 0) return false;

    int oldest  = history_oldest_turn(&g->history);
    int newest  = history_newest_turn(&g->history);
    int min_off = oldest - newest;
    if (g->review_offset > 0)        g->review_offset = 0;
    if (g->review_offset < min_off)  g->review_offset = min_off;

    return true;
}

void input_poll(PlaydateAPI* pd, GameState* g) {
    PDButtons cur, push, rel;
    pd->system->getButtonState(&cur, &push, &rel);
    (void)rel;
    float crank = pd->system->getCrankChange();

    if (g->mode == GAME_MODE_TITLE) {
        handle_title(pd, g, push, crank);
        return;
    }

    /* Deferred AI: previous frame set ai_pending while drawing the hourglass.
     * Run synchronously now; the hourglass frame stays on screen for the
     * duration of the search since Playdate only swaps buffers when update
     * returns. */
    g->ai_thinking_frames++;
    if (g->mode == GAME_MODE_AI_THINKING && g->ai_pending) {
        game_run_ai(g);
        return;
    }
    if (g->mode == GAME_MODE_AI_THINKING) return;

    /* Time-travel works in FREE / PIECE_SELECTED / GAME_OVER. */
    if (g->mode != GAME_MODE_AI_THINKING) {
        if (handle_time_travel(g, cur, crank)) return;
    }

    if (g->mode == GAME_MODE_GAME_OVER) {
        if (push & kButtonA) game_to_title(g);
        return;
    }

    if (push & kButtonUp)    game_move_cursor(g,  0, +1);
    if (push & kButtonDown)  game_move_cursor(g,  0, -1);
    if (push & kButtonLeft)  game_move_cursor(g, -1,  0);
    if (push & kButtonRight) game_move_cursor(g, +1,  0);

    if (push & kButtonA) game_action_a(g);
    if (push & kButtonB) game_action_b(g);
}
