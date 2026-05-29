

#ifndef QUORIDOR_H_
#define QUORIDOR_H_

#include "../solver.h"




typedef enum
{
	CELL_UNCHECKED,
	CELL_BAD,
	CELL_WAVE_END,
} cell_status_t;

typedef struct
{
	uint8_t dist;
	uint8_t status;
} cell_t;



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
	cell_t map[81];
}  quor_pos_t;

extern solver_t QUOR_SOLVER;

bool blocked_up(__int128 b, __int128 gates);
bool blocked_down(__int128 b, __int128 gates);
bool blocked_left(__int128 b, __int128 gates);
bool blocked_right(__int128 b, __int128 gates);

#endif	//QUORIDOR_H_
