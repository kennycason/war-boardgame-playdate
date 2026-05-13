#include "input.h"

void input_poll(PlaydateAPI* pd, GameState* g) {
    PDButtons current, pushed, released;
    pd->system->getButtonState(&current, &pushed, &released);
    (void)current;
    (void)released;

    if (g->mode == GAME_MODE_TITLE) {
        if (pushed & (kButtonA | kButtonB)) {
            game_reset(g);
        }
        return;
    }

    /* Cursor movement (grid steps). Y on screen is inverted vs board Y. */
    if (pushed & kButtonUp)    game_move_cursor(g,  0, +1);
    if (pushed & kButtonDown)  game_move_cursor(g,  0, -1);
    if (pushed & kButtonLeft)  game_move_cursor(g, -1,  0);
    if (pushed & kButtonRight) game_move_cursor(g, +1,  0);

    if (pushed & kButtonA) game_action_a(g);
    if (pushed & kButtonB) game_action_b(g);
}
