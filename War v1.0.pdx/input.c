#include "input.h"

/* Crank sensitivity — degrees of rotation required for one "click". Tuned so
 * the gesture is deliberate: a full 360° spin moves AI level by 1 and history
 * by 3. */
#define CRANK_PER_AI_LEVEL  360.0f
#define CRANK_PER_HISTORY   120.0f

/* On the title screen the crank cycles AI level. */
static void handle_title(GameState* g, PDButtons pushed, float crank) {
    if (pushed & kButtonUp)    game_title_nav(g, -1);
    if (pushed & kButtonDown)  game_title_nav(g, +1);
    if (pushed & kButtonLeft)  game_title_change(g, -1);
    if (pushed & kButtonRight) game_title_change(g, +1);

    static float accum = 0.0f;
    accum += crank;
    while (accum >= CRANK_PER_AI_LEVEL) {
        if (g->settings.ai_level < AI_LEVEL_MAX) g->settings.ai_level++;
        accum -= CRANK_PER_AI_LEVEL;
    }
    while (accum <= -CRANK_PER_AI_LEVEL) {
        if (g->settings.ai_level > AI_LEVEL_MIN) g->settings.ai_level--;
        accum += CRANK_PER_AI_LEVEL;
    }

    if (pushed & kButtonA) game_title_select(g);
}

/* Plain crank → review past turns.
 *   counter-clockwise (negative crank) → back in time (offset decreases)
 *   clockwise         (positive crank) → forward in time (offset increases) */
static bool handle_review_crank(GameState* g, float crank) {
    if (crank == 0.0f && g->review_offset == 0) return false;
    g->review_crank_accum += crank;
    while (g->review_crank_accum >= CRANK_PER_HISTORY) {
        g->review_offset += 1;
        g->review_crank_accum -= CRANK_PER_HISTORY;
    }
    while (g->review_crank_accum <= -CRANK_PER_HISTORY) {
        g->review_offset -= 1;
        g->review_crank_accum += CRANK_PER_HISTORY;
    }
    int oldest  = history_oldest_turn(&g->history);
    int newest  = history_newest_turn(&g->history);
    int min_off = oldest - newest;
    if (g->review_offset > 0)       g->review_offset = 0;
    if (g->review_offset < min_off) g->review_offset = min_off;
    return true;
}

/* A-held + crank → cycle AI level. Sets a_used_for_crank so the deferred A
 * release doesn't also fire a regular A action. */
static void handle_a_crank(GameState* g, float crank) {
    g->a_crank_accum += crank;
    while (g->a_crank_accum >= CRANK_PER_AI_LEVEL) {
        if (g->settings.ai_level < AI_LEVEL_MAX) g->settings.ai_level++;
        g->a_crank_accum -= CRANK_PER_AI_LEVEL;
        g->a_used_for_crank = true;
    }
    while (g->a_crank_accum <= -CRANK_PER_AI_LEVEL) {
        if (g->settings.ai_level > AI_LEVEL_MIN) g->settings.ai_level--;
        g->a_crank_accum += CRANK_PER_AI_LEVEL;
        g->a_used_for_crank = true;
    }
}

void input_poll(PlaydateAPI* pd, GameState* g) {
    PDButtons cur, push, rel;
    pd->system->getButtonState(&cur, &push, &rel);
    float crank = pd->system->getCrankChange();

    if (g->mode == GAME_MODE_TITLE) {
        handle_title(g, push, crank);
        return;
    }

    /* AI search runs one ply per frame so the screen can redraw (animated
     * dots, etc.) between depths. The hourglass frame from after_move_applied
     * shows on the first iteration. */
    if (g->mode == GAME_MODE_AI_THINKING) {
        g->ai_thinking_frames++;
        game_ai_step(g);
        return;
    }

    /* A-button hold tracking (deferred press-on-release). */
    bool a_now      = (cur & kButtonA) != 0;
    bool a_released = (rel & kButtonA) != 0;

    if (a_now) {
        if (crank != 0.0f) handle_a_crank(g, crank);
    } else {
        if (a_released) {
            if (!g->a_used_for_crank) {
                /* Real tap — fire A action now. */
                game_action_a(g);
            }
            g->a_used_for_crank = false;
            g->a_crank_accum    = 0.0f;
        } else {
            /* A not held: plain crank goes to review history. */
            handle_review_crank(g, crank);
        }
    }
    g->a_held = a_now;

    /* B: cancel selection / restart from game-over. Acts on press. */
    if (push & kButtonB) {
        if (g->mode == GAME_MODE_GAME_OVER) {
            game_to_title(g);
        } else if (g->review_offset != 0) {
            /* Snap back to live without branching. */
            g->review_offset      = 0;
            g->review_crank_accum = 0.0f;
        } else {
            game_action_b(g);
        }
    }

    if (g->mode == GAME_MODE_GAME_OVER) return;

    /* D-pad cursor — disabled while A is held (treats hold as "I'm cranking"). */
    if (!a_now) {
        if (push & kButtonUp)    game_move_cursor(g,  0, +1);
        if (push & kButtonDown)  game_move_cursor(g,  0, -1);
        if (push & kButtonLeft)  game_move_cursor(g, -1,  0);
        if (push & kButtonRight) game_move_cursor(g, +1,  0);
    }
}
