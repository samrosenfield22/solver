

#ifndef TTT_H_
#define TTT_H_

#include "../solver.h"

typedef enum
{
	EMPTY = 0,
	X_PIECE = 1,
	O_PIECE = 2,
} piece_type_t;

typedef struct
{
	bool whosemove;
	uint8_t spaces[9];
} ttt_pos_t;

extern solver_t TTT_SOLVER;


#endif	//TTT_H_
