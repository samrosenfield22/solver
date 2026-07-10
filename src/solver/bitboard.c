

#include "bitboard.h"
#include "zobrist.h"

#include <assert.h>

int GAME_W, GAME_H, GAME_TILES_CT;
uint64_t GAME_MASK;

void bb64_init(int w, int h, uint64_t mask)
{
	GAME_W = w;
	GAME_H = h;
	GAME_TILES_CT = w*h;
	GAME_MASK = mask;

	//zobrist_init()
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
	}

	return bb | nbit;
}

//uint64_t bb64_hash(uint64_t bb)
{

}
