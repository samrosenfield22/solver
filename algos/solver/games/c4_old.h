

#ifndef C4_H_
#define C4_H_

#include "../solver.h"



typedef struct
{
	uint8_t columns_filled[7];
	uint8_t columns_color[7];
	bool whosemove;
} c4_pos_t;

extern solver_t C4_SOLVER;


#endif	//C4_H_
