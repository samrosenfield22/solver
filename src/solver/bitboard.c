

#include "bitboard.h"
#include "zobrist.h"

#include <assert.h>

int GAME_W=7, GAME_H=6, GAME_TILES_CT=42;
//uint64_t GAME_MASK;
uint64_t GAME_MASK = 0b0111111011111101111110111111011111101111110111111;

void bb64_init(int w, int h, uint64_t mask)
{
	GAME_W = w;
	GAME_H = h;
	GAME_TILES_CT = w*h;
	GAME_MASK = mask;
}

uint64_t bb64_get_open(uint64_t bb)
{
	return (~bb) & GAME_MASK;
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
	assert(!(bb & nbit));
	uint64_t new_bb = bb | nbit;
	if(hash)
	{
		int index = __builtin_ctzll(nbit);
		index -= __builtin_popcountll((nbit-1) & ~GAME_MASK);
		if(!whosemove)
			index += GAME_TILES_CT;

		zobrist_place(hash, index);

		//optional check, compare to hash()
	}

	return new_bb;
}
