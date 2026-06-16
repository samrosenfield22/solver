#include "shared.h"
solver_t *solver;

uint32_t *gdata_get_hash(gdata_t *gd)
{
	if(solver->uses_zobrist)
		return &(gd->hash);
	else
		return NULL;
}
