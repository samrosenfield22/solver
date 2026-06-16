/*

#ifndef TRANSPOSITION_H_
#define TRANSPOSITION_H_

#include <stdint.h>
#include <stdbool.h>

enum
{
	BOUND_EXACT,
	BOUND_UPPER,
	BOUND_LOWER,
};

typedef struct
{
	float score;
	uint8_t search_depth;
	bool full;
	uint8_t bound;
	uint8_t best_move;
	//age??
} trans_value_t;

void tt_create(void);
int tt_add(gdata_t *gd, result_t *result, int depth,
	int bound, int best_move);
bool tt_get(trans_value_t *value, gdata_t *gd, int depth);

#endif	//TRANSPOSITION_H_
*/
