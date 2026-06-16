

#ifndef C4_H_
#define C4_H_

#include "../solver.h"



typedef struct
{
	uint64_t x;
	uint64_t filled;

	uint64_t x_wmap, opp_wmap;
	bool won;
} c4_pos_t;

extern solver_t C4_SOLVER;


#endif	//C4_H_
