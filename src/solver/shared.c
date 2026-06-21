#include "shared.h"
#include <string.h>

solver_t *solver;
size_t gdata_size;

uint64_t *gdata_get_hash(gdata_t *gd)
{
	if(solver->uses_zobrist)
		return &(gd->hash);
	else
		return NULL;
}

bool make_new_move(gdata_t *child, gdata_t *gd, int move)
{

	memcpy(child, gd, gdata_size);

	uint64_t *hp = gdata_get_hash(child);
	solver->make_move(&(child->pos), move, hp);
	child->move_index = move;

	return true;	//always?
}
