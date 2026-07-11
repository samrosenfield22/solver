

#include "bitboard.h"
#include "zobrist.h"
#include "../utils/utils.h"

#include <stdio.h>
#include <assert.h>

int GAME_W, GAME_H, GAME_TILES_CT;
uint64_t GAME_MASK, HIGHEST_BIT=0;

void bb64_init(int w, int h, uint64_t mask)
{
	assert(w > 0);
	assert(h > 0);
	assert(mask);
	assert(__builtin_popcountll(mask) == w*h);

	GAME_W = w;
	GAME_H = h;
	GAME_TILES_CT = w*h;
	GAME_MASK = mask;

	for(uint64_t b=(((uint64_t)1)<<63); b; b>>=1)
	{
		if(b & mask)
		{
			HIGHEST_BIT = b;
			break;
		}
	}
	assert(HIGHEST_BIT);
	//printf("highest bit is %s\n", sprintbig(HIGHEST_BIT, "%b"));
	//exit(0);

	//zobrist_init(2*__builtin_popcountll(mask), 0);
}

uint64_t bb64_get_open(uint64_t bb)
{
	return (~bb) & GAME_MASK;
}

int bb64_get_open_ct(uint64_t bb)
{
	return GAME_TILES_CT - __builtin_popcountll(bb);
}

bool bb64_is_full(uint64_t bb)
{
	return ((bb & GAME_MASK) == GAME_MASK);
}

bool bb64_is_empty(uint64_t bb)
{
	return ((bb & GAME_MASK) == 0);
}

//has to be called BEFORE flipping the whosemove bit/flag
uint64_t bb64_place(uint64_t bb, uint64_t nbit, uint64_t *hash, bool whosemove)
{
	assert(nbit);
	assert(!(bb & nbit));
	if(hash)
	{
		//int index = __builtin_ctzll(nbit);
		//index -= __builtin_popcountll((nbit-1) & ~GAME_MASK);
		int index = __builtin_popcountll((nbit-1) & GAME_MASK);
		if(!whosemove)
			index += GAME_TILES_CT;

		zobrist_place(hash, index);

		//optional check, compare to hash()
		//uint64_t check_hash = bb64_hash(bb, whosemove);
		//assert(*hash == check_hash);
	}

	return bb | nbit;
}

int bb64_make_place_movelist(sorter_t *sorter, uint64_t bb)
{
	uint64_t moves = bb64_get_open(bb);
	int ct = 0, index = 0;
	for(uint64_t b=1; b<HIGHEST_BIT; b<<=1)
	{
		if(!(b & GAME_MASK))
			continue;

		if(b & moves)
		{
			sorter[ct].move = index;
			sorter[ct].score = 0;
			ct++;
		}
		index++;
	}

	return ct;
}

/*uint64_t bb64_hash(uint64_t bb, bool whosemove)
{
	uint64_t hash = 0;
	int index = 0;
	for(uint64_t b=1; b<=HIGHEST_BIT; b<<=1)
	{
		if(!(b & GAME_MASK))
			continue;

		if(b & bb)
			zobrist_place(&hash, index + whosemove? GAME_TILES_CT : 0);
		index++;
	}

	return hash;
}*/
