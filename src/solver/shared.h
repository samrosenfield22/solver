

#ifndef SHARED_H_
#define SHARED_H_

#include <stdbool.h>

typedef struct
{
	float score;
	bool full;
	int best_move;
} result_t;

#include "solver.h"	//eww
extern solver_t *solver;

typedef struct
{
	uint32_t hash;
	float score;
	int move_index;
	//uint8_t move_ct;
	uint8_t pos[];
} gdata_t;

uint32_t *gdata_get_hash(gdata_t *gd);

#endif	//SHARED_H_
