#ifndef WAR_RENDER_H
#define WAR_RENDER_H

#include "pd_api.h"
#include "game.h"

/* Layout constants — exposed so input/main can map screen ↔ board. */
#define BOARD_ORIGIN_X  4
#define BOARD_ORIGIN_Y  10
#define TILE_SIZE       20
#define BOARD_PX_SIZE   (TILE_SIZE * BOARD_DIM)

#define SIDEBAR_X       (BOARD_ORIGIN_X + BOARD_PX_SIZE + 6)
#define SIDEBAR_W       (LCD_COLUMNS - SIDEBAR_X - 2)

void render_init(PlaydateAPI* api);
void render_frame(GameState* g);

#endif
