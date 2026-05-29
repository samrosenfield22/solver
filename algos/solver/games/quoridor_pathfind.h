

#ifndef QUORIDOR_PATHFIND_H_
#define QUORIDOR_PATHFIND_H_

#include "quoridor.h"

//
void map_init(cell_t *map);
void update_dists(cell_t *map, int *next_to_gate,
	__int128 h, __int128 v);

#endif	//QUORIDOR_PATHFIND_H_
