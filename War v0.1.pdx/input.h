#ifndef WAR_INPUT_H
#define WAR_INPUT_H

#include "pd_api.h"
#include "game.h"

/* Polls the Playdate buttons and updates the game state. */
void input_poll(PlaydateAPI* pd, GameState* g);

#endif
