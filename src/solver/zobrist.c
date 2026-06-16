

#include "zobrist.h"
#include "../utils/utils.h"

#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

//#define ZOBRIST_LEN	(148)
int ZOBRIST_LEN;

//typedef uint64_t z_data_t;

//bool zobrist_computed = false;
//uint32_t zobrist_strings[ZOBRIST_LEN];
uint32_t *zobrist_strings = NULL;

void zobrist_init(int len, int zseed)
{
	if(zobrist_strings)
		return;

	zobrist_strings = mem_malloc(len * sizeof(*zobrist_strings));
	assert(zobrist_strings);

	//zobrist_computed = true;
	ZOBRIST_LEN = len;

	srand(zseed);
	for(int i=0; i<ZOBRIST_LEN; i++)
	{
		zobrist_strings[i] = rand()<<16 | rand();
	}
}

void zobrist_free(void)
{
	mem_free(zobrist_strings);
	zobrist_strings = NULL;
}

void zobrist_place(uint32_t *h, int n)
{
	assert(n < ZOBRIST_LEN);
	*h ^= zobrist_strings[n];
}

void zobrist_move(uint32_t *h, int to, int from)
{
	assert(from < ZOBRIST_LEN);
	assert(to < ZOBRIST_LEN);
	*h ^= zobrist_strings[to];
	*h ^= zobrist_strings[from];
}

void zobrist_move_capture(uint32_t *h, int to, int from, int captured)
{
	assert(from < ZOBRIST_LEN);
	assert(to < ZOBRIST_LEN);
	assert(captured < ZOBRIST_LEN);

	*h ^= zobrist_strings[to];
	*h ^= zobrist_strings[from];
	*h ^= zobrist_strings[captured];
}
