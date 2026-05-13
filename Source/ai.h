#ifndef WAR_AI_H
#define WAR_AI_H

#include <stdbool.h>
#include "types.h"
#include "board.h"
#include "move.h"

/* Static evaluation — positive favors `player`, negative favors opponent. */
int  ai_evaluate(const Board* b, Player player);

/* Chooses the best move for board.current_player at the given difficulty.
 * Returns true if a move was found (false only if the player has no legal moves).
 * Blocking — caller should display a thinking indicator first. */
bool ai_pick_move(Board* b, AIDifficulty difficulty, Move* out);

/* Counts of nodes evaluated in the last call — for tuning. */
int  ai_last_node_count(void);

#endif
