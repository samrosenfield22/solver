

#ifndef QUORIDOR_H_
#define QUORIDOR_H_

#include "../solver.h"


typedef struct
{
	uint8_t x, y, gates;
	__int128 token;
} quor_player_t;

typedef struct
{
	uint64_t horiz, vert;
	bool whosemove;
	quor_player_t p1, p2;

	///// aux
	//me_aux, opp_aux
}  quor_pos_t;

extern solver_t QUOR_SOLVER;

#endif	//QUORIDOR_H_
