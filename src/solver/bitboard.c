

#include "bitboard.h"
#include "zobrist.h"
#include "../utils/utils.h"

#include <stdio.h>
#include <assert.h>

int GAME_W, GAME_H, GAME_TILES_CT;
uint64_t GAME_MASK, HIGHEST_BIT=0;

//drawing
int DRAW_DIR1=DRAW_RIGHT_DIR, DRAW_DIR2=DRAW_DOWN_DIR;
int DRAW_X_SPACE=2, DRAW_Y_SPACE=2;

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

void bb64_init_draw(int dir_1, int dir_2, int x_space, int y_space)
{
	DRAW_DIR1 = dir_1;
	DRAW_DIR2 = dir_2;
	DRAW_X_SPACE = x_space;
	DRAW_Y_SPACE = y_space;
}

uint64_t bb64_get_open(uint64_t bb)
{
	return (~bb) & GAME_MASK;
}

int bb64_get_open_ct(uint64_t bb)
{
	int placed_ct = __builtin_popcountll(bb & GAME_MASK);
	return GAME_TILES_CT - placed_ct;
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
uint64_t bb64_place(uint64_t bb, uint64_t nbit, uint64_t *hash,
	bool whosemove)
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

bool jump_get_params(int dir, int *d)
{
	switch(dir)
	{
		case JUMP_U:	*d = GAME_W + 1; return false; break;
		case JUMP_D:	*d = GAME_W + 1; return true; break;
		case JUMP_R:	*d = 1;			return false; break;
		case JUMP_L:	*d = 1;			return true; break;
		case JUMP_UL:	*d = GAME_W;		return false; break;
		case JUMP_UR:	*d = GAME_W + 2; return false; break;
		case JUMP_DL:	*d = GAME_W + 2;	return true; break;
		case JUMP_DR:	*d = GAME_W;		return true; break;
		default: assert(0); return true;
	}
}

//assumes 1 gap bit
uint64_t bb64_get_jumps(uint64_t x, uint64_t filled, int dir)
{
	int d;
	bool shl = jump_get_params(dir, &d);

	if(shl)
		return x
		& (((x^filled)&GAME_MASK)<<d)
		& (((~filled)&GAME_MASK)<<(d<<1));
	else
		return x
		& (((x^filled)&GAME_MASK)>>d)
		& (((~filled)&GAME_MASK)>>(d<<1));
}

bool bb64_is_jump_legal(uint64_t b, uint64_t x, uint64_t filled, int dir)
{
	assert(b & x);
	/*if(!((b<<1) & (x ^ filled)))
		return false;
	if((b<<2) & filled)
		return false;
	return true;*/

	//or...
	return bb64_get_jumps(x, filled, dir) & b;
}

uint64_t bb64_get_jumps_ortho(uint64_t x, uint64_t filled)
{
	return bb64_get_jumps(x, filled, JUMP_U)
		| bb64_get_jumps(x, filled, JUMP_D)
		| bb64_get_jumps(x, filled, JUMP_L)
		| bb64_get_jumps(x, filled, JUMP_R);
}

uint64_t bb64_get_jumps_diag(uint64_t x, uint64_t filled)
{
	return bb64_get_jumps(x, filled, JUMP_UL)
		| bb64_get_jumps(x, filled, JUMP_UR)
		| bb64_get_jumps(x, filled, JUMP_DL)
		| bb64_get_jumps(x, filled, JUMP_DR);
}

uint64_t bb64_get_jumps_all(uint64_t x, uint64_t filled)
{
	return bb64_get_jumps_ortho(x, filled)
		| bb64_get_jumps_diag(x, filled);
}

void bb64_jump(uint64_t b, uint64_t *x, uint64_t *filled,
	uint64_t dir, uint64_t *hash, bool whosemove)
{
	int d;
	bool shl = jump_get_params(dir, &d);
	uint64_t remove;

	if(shl)
	{
		*x &= ~b;
		remove = (b<<(d<<1));
		*x |= remove;
		//*x |= (b<<(d<<1));
		*filled &= ~(b | (b<<d));
		*filled |= (b<<(d<<1));
	}
	else
	{
		*x &= ~b;
		remove = (b>>(d<<1));
		*x |= remove;
		//*x |= (b>>(d<<1));
		*filled &= ~(b | (b>>d));
		*filled |= (b>>(d<<1));
	}

	//hash
	if(hash)
	{
		int index = __builtin_popcountll((b-1) & GAME_MASK);
		if(!whosemove)
			index += GAME_TILES_CT;

		int dif;
		if(shl)
			dif = __builtin_popcountll(remove - b);
		else
			dif = __builtin_popcountll(b - remove) * -1;

		zobrist_place(hash, index);
		zobrist_place(hash, index + dif);
		zobrist_place(hash, index + (dif<<1));

		//optional check, compare to hash()
		//uint64_t check_hash = bb64_hash(bb, whosemove);
		//assert(*hash == check_hash);
	}
}

//builds movelist for all open spaces
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

void bb64_draw(uint64_t bb, const char *piece)
{
	int x, y;
	int x_start=0, y_start=0;
	int x_1_incr, x_2_incr, y_1_incr, y_2_incr;
	switch(DRAW_DIR1)
	{
		case DRAW_UP_DIR:
			x_1_incr = 0;
			y_1_incr = -1;
			y_start = GAME_H-1;
			break;

		case DRAW_DOWN_DIR:
			x_1_incr = 0;
			y_1_incr = 1;
			y_start = 0;
			break;

		case DRAW_LEFT_DIR:
			x_1_incr = 1;
			y_1_incr = 0;
			x_start = GAME_W-1;
			break;

		case DRAW_RIGHT_DIR:
			x_1_incr = 1;
			y_1_incr = 0;
			x_start = 0;
			break;

		default: assert(0); exit(0);
	}

	switch(DRAW_DIR2)
	{
		case DRAW_UP_DIR:
			x_2_incr = 0;
			y_2_incr = -1;
			y_start = GAME_H-1;
			break;

		case DRAW_DOWN_DIR:
			x_2_incr = 0;
			y_2_incr = 1;
			y_start = 0;
			break;

		case DRAW_LEFT_DIR:
			x_2_incr = 1;
			y_2_incr = 0;
			x_start = GAME_W-1;
			break;

		case DRAW_RIGHT_DIR:
			x_2_incr = 1;
			y_2_incr = 0;
			x_start = 0;
			break;

		default: assert(0); exit(0);
	}

	x = x_start;
	y = y_start;
	for(uint64_t b=1; b<=HIGHEST_BIT; b<<=1)
	{
		if(!(b & GAME_MASK))
			continue;

		if(b & bb)
		{
			//draw
			int x_draw = 61 + x*DRAW_X_SPACE;
			int y_draw = 19 + y*DRAW_Y_SPACE;
			window_unfocus();
			term_move_cursor(x_draw, y_draw);
			printf("%s%s", TERM_WHITE, TERM_BLACK_BG);
			printf(piece);
		}

		x += x_1_incr;
		y += y_1_incr;

		//check out of bounds
		if(x<0 || x>=GAME_W)
		{
			x = x_start;
			y += y_2_incr;
		}
		if(y<0 || y>=GAME_H)
		{
			y = y_start;
			x += x_2_incr;
		}
	}
}

uint64_t bb64_hash(uint64_t bb, bool whosemove)
{
	uint64_t hash = 0;
	int index = 0;
	const int offset = whosemove? GAME_TILES_CT : 0;

	for(uint64_t b=1; b<=HIGHEST_BIT; b<<=1)
	{
		if(!(b & GAME_MASK))
			continue;

		if(b & bb)
			zobrist_place(&hash, index + offset);
		index++;
	}

	return hash;
}
