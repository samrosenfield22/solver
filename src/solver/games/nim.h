

#ifndef NIM_H_
#define NIM_H_

#include "../solver.h"

typedef struct
{
	bool whosemove;
	uint8_t piles[3];
	//bool whosemove;
} nim_pos_t;

extern solver_t NIM_SOLVER;


#endif	//NIM_H_
