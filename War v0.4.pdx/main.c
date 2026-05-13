#include <stdio.h>
#include "pd_api.h"
#include "game.h"
#include "render.h"
#include "input.h"

static PlaydateAPI* pd = NULL;
static GameState    game;

static int update(void* ud) {
    (void)ud;
    input_poll(pd, &game);
    render_frame(&game);
    return 1;
}

#ifdef _WINDLL
__declspec(dllexport)
#endif
int eventHandler(PlaydateAPI* playdate, PDSystemEvent event, uint32_t arg) {
    (void)arg;
    if (event == kEventInit) {
        pd = playdate;
        render_init(pd);
        game_init(&game);
        game.mode = GAME_MODE_TITLE;
        pd->display->setRefreshRate(30);
        pd->system->setUpdateCallback(update, NULL);
    }
    return 0;
}
