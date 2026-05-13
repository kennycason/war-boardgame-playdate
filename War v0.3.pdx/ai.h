#ifndef WAR_AI_H
#define WAR_AI_H

#include <stdbool.h>
#include "types.h"
#include "board.h"
#include "move.h"

/* Static evaluation — positive favors `player`, negative favors opponent. */
int  ai_evaluate(const Board* b, Player player);

/* Maps a level (1..9) to a target search depth and a noise percentage. */
void ai_level_config(int level, int* out_depth, int* out_noise_pct);

/* Searches to a specific depth (1..N). Used both by the all-in-one pick and
 * the incremental iterative-deepening driver. Returns true on success. */
bool ai_pick_move_at_depth(Board* b, int depth, int noise_pct, Move* out);

/* Convenience: full search at the given level. Blocking. */
bool ai_pick_move(Board* b, int level, Move* out);

/* Counts of nodes evaluated in the last call — for tuning. */
int  ai_last_node_count(void);

#endif
