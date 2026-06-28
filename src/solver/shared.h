

#ifndef SHARED_H_
#define SHARED_H_

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
	float score;
	bool full;
	bool has_tt;
	int best_move;
} result_t;

#include "solver.h"	//eww
extern solver_t *solver;
extern size_t gdata_size;

typedef struct
{
	uint64_t hash;
	float score;
	int move_index;
	//uint8_t move_ct;
	uint8_t pos[];
} gdata_t;

uint64_t *gdata_get_hash(gdata_t *gd);
bool make_new_move(gdata_t *child, gdata_t *gd, int move);

#endif	//SHARED_H_
