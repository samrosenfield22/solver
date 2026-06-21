

#include "zobrist.h"
#include "../utils/utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

int ZOBRIST_LEN;

//typedef uint64_t z_data_t;

//bool zobrist_computed = false;
uint64_t *zobrist_strings = NULL;

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
		for(int z=0; z<4; z++)
		{
			zobrist_strings[i] <<= 16;
			zobrist_strings[i] |= rand();
		}
		//zobrist_strings[i] = rand()<<48 | rand()<<32 | rand()<<16 | rand();
		//printf("zstr %d is %s\n", i, sprintbig(zobrist_strings[i], "%b"));
	}
	//exit(0);
}

void zobrist_free(void)
{
	mem_free(zobrist_strings);
	zobrist_strings = NULL;
}

void zobrist_place(uint64_t *h, int n)
{
	assert(n < ZOBRIST_LEN);
	*h ^= zobrist_strings[n];
	//*(uint32_t *)h ^= (uint32_t)zobrist_strings[n];
}

void zobrist_move(uint64_t *h, int to, int from)
{
	assert(from < ZOBRIST_LEN);
	assert(to < ZOBRIST_LEN);
	*h ^= zobrist_strings[to];
	*h ^= zobrist_strings[from];
}

void zobrist_move_capture(uint64_t *h, int to, int from, int captured)
{
	assert(from < ZOBRIST_LEN);
	assert(to < ZOBRIST_LEN);
	assert(captured < ZOBRIST_LEN);

	*h ^= zobrist_strings[to];
	*h ^= zobrist_strings[from];
	*h ^= zobrist_strings[captured];
}
