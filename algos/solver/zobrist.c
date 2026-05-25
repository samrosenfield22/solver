

#include "zobrist.h"

#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#define ZOBRIST_LEN	(85)
//int ZOBRIST_LEN;

bool zobrist_computed = false;
uint32_t zobrist_strings[ZOBRIST_LEN];

void zobrist_init(int zseed)
{
	if(zobrist_computed)
		return;
	zobrist_computed = true;
	
	srand(zseed);
	for(int i=0; i<ZOBRIST_LEN; i++)
	{
		zobrist_strings[i] = rand()<<16 | rand();
	}
}

void zobrist_place(uint32_t *h, int n)
{
	*h ^= zobrist_strings[n];
}

void zobrist_move_piece(uint32_t *h, int to, int from)
{
	*h ^= zobrist_strings[to];
	*h ^= zobrist_strings[from];
}

void zobrist_move_capture(uint32_t *h, int to, int from, int captured)
{
	*h ^= zobrist_strings[to];
	*h ^= zobrist_strings[from];
	*h ^= zobrist_strings[captured];
}
