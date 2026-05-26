

#ifndef QUORIDOR_H_
#define QUORIDOR_H_

#include "../solver.h"


typedef struct
{
	uint8_t x, y, gate_ct;
	__int128 token;
} quor_player_t;

typedef struct
{
	__attribute__((aligned(16))) __int128 horiz, vert, gates;
	bool whosemove;
	quor_player_t p1, p2;

	///// aux
	//me_aux, opp_aux
}  quor_pos_t;

extern solver_t QUOR_SOLVER;

#endif	//QUORIDOR_H_
