

#include "c4.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <assert.h>

#include "../zobrist.h"
#include "../../utils/utils.h"

//#define SHOW_WIN_TILES

float estimate_color(uint64_t x, uint64_t opp, uint64_t filled,
	uint64_t wmap, uint64_t opp_wmap, bool verbose);
void get_win_maps(c4_pos_t *p);
//void get_win_map(uint64_t *wmap, uint64_t x, uint64_t filled);
void c4_draw_full(void *pos, int last_move);
uint32_t c4_hash(void *key, size_t size);

#define WHOSEMOVE_BIT	(((uint64_t)1)<<63)
#define NO_WIN_MAP		(0xFFFFFFFFFFFFFFFF)

//tried srand seeds from 0 through 50
//best: 17 (43 collisions at d=18)
//worst: 19 (122)
#define ZOBRIST_SEED	(9)

c4_pos_t C4_INIT_POS =
{
	.x = 0,
	.filled = WHOSEMOVE_BIT,

	.x_wmap = 0, .opp_wmap = 0,
	.won = false,
};

char *print64(uint64_t n)
{
	static char buf[64];
	snprintf(buf, 63, "0x%016" PRIx64 "", n);
	return buf;
}

uint8_t get_col(uint64_t col, int index)
{
	uint8_t c = 0b111111;
	c &= col >> (7*index);
	return c;
}

//#define c4_ok(pos)	true
bool c4_ok(c4_pos_t *p)
{
	for(int i=0; i<7; i++)
	{
		if(get_col(p->filled, i) & 0b11000000)
			return false;
		if(get_col(p->x, i) & 0b11000000)
			return false;
	}

	if(p->x_wmap != NO_WIN_MAP)
		if(p->x_wmap & p->filled)
			return false;

	if(p->opp_wmap != NO_WIN_MAP)
		if(p->opp_wmap & p->filled)
			return false;

	return true;
}

bool c4_whosemove(void *pos)
{
	//assert(c4_ok(pos));
	c4_pos_t *p = pos;
	//return p->whosemove;
	return p->filled & WHOSEMOVE_BIT;
}


//returns the bit required to make the given move (or with
//both x and filled)
uint64_t move_bit(c4_pos_t *p, int index)
{
	uint64_t col = get_col(p->filled, index)+1;
	col &= 0b111111;
	return col<<(7*index);
}

bool is_win(uint64_t x)
//bool is_win(c4_pos_t *p)
{
	//if(p->x_wmap != NO_WIN_MAP)
	//	return (p->x & p->x_wmap);
	//get_win_map(p, p->x);
	//get_win_map(&p->x_wmap, p->x, p->filled);
	//return (p->x & p->x_wmap);
	//uint64_t x = p->x;	//compiler

	//horizontal
	uint64_t half = x & (x<<7);
	if(half & (half<<14))
		return true;


	//fd diag
	half = x & (x<<8);
	if(half & (half<<16))
		return true;

	//bk diag
	half = x & (x<<6);
	if(half & (half<<12))
		return true;

	//vertical
	half = x & (x<<1);
	if(half & (half<<2))
		return true;

	//nope
	return false;
}

bool c4_player_can_win(uint64_t x, uint64_t filled)
{




	/*if we have a token in 3, and the opp has blocking
	tokens in both (1,2) and (4,5), that path can't win

	if any path can win, return true
	*/

	uint64_t opp = x ^ filled;
	uint64_t x_middle = x & 0b0111111000000000000000000000;
	uint64_t opp_left = opp & 0b011111101111110000000;
	uint64_t opp_right = opp & 0b011111101111110000000000000000000000000000;
	int shifts[] = {6, 7, 8};

	for(int i=3; i<7; i++)
	{
		for(int s=0; s<3; s++)
		{
			int sh = shifts[s];
			uint64_t x_mask = 0b111111;
			x_mask <<= (i & 7);
			uint64_t x_col = x & x_mask;

			uint64_t blockers = (opp<<sh)
				| (opp<<(sh*2))
				| (opp<<(sh*3));

			if(x_col != blockers)
				return true;

			//left
			uint64_t l = (opp_left>>sh) | (opp_left>>(2*sh));
			if(sh==6)	//down diag
				l |= 0b110000000000000000000;
			else if(sh==8)	//up diag
				l |= 0b000001100000000000000;
			uint64_t blocked = l & x_middle;
			if(blocked != x_middle)
				return true;

			//right
			uint64_t r = (opp_right<<sh) | (opp_left<<(2*sh));
			if(sh==6)	//down diag
				l |= 0b000001100000000000000;
			else if(sh==8)	//up diag
				l |= 0b110000000000000000000;
			blocked = r & x_middle;
			if(blocked != x_middle)
				return true;
		}
	}

	return false;

	/*uint64_t b = 1;
	b <<= 21;	//middle bottom
	uint64_t end = 1;
	end <<= 27;
	for(; b>end; b<<=1)
	{
		if(!b & x)
			continue;

		int shifts[] = {6, 7, 8};
		for(int s=0; s<3; s++)
		{
			int sh = shifts[s];
			for(int i=0; i<3; i++)
			{
				uint64_t next = b << sh;
				if(!(next & opp) && not off edge)
					//return true;
			}
		}
	}*/
}

void make_paths(uint64_t *paths)
{
	//48 paths (24 horiz, 12 up diag, 12 down diag)

	//horiz
	for(int c=0; c<4; c++)
	{
		for(int r=0; r<6; r++)
		{
			uint64_t st = 0b1;
			st <<= (7*c);
			st <<= r;

			uint64_t pt = st | (st<<7) | (st<<14) | (st<<21);
			int index = c*6 + r;
			paths[index] = pt;
		}
	}

	//up diag
	for(int c=0; c<4; c++)
	{
		for(int r=0; r<3; r++)
		{
			uint64_t st = 0b1;
			st <<= (7*c);
			st <<= r;

			uint64_t pt = st | (st<<8) | (st<<16) | (st<<24);
			int index = c*3 + r;
			index += 24;
			paths[index] = pt;
		}
	}

	//down diag
	for(int c=0; c<4; c++)
	{
		for(int r=5; r>2; r--)
		{
			uint64_t st = 0b1;
			st <<= (7*c);
			st <<= r;

			uint64_t pt = st | (st<<6) | (st<<12) | (st<<18);
			int index = c*3 + r-3;
			index += 36;
			paths[index] = pt;
		}
	}

	/*term_move_cursor(0, 0);
	for(int i=0; i<48; i++)
		printf("path %d: %s\n", i, sprintbig(paths[i], "%b"));
	exit(0);*/
}

bool c4_win_impossible(c4_pos_t *p)
{
	//only relevant if column 3 is completely full
	/*if(!(p->filled & 0b0111111000000000000000000000))
	{
		//printf("col 3 not full!\n");
		//exit(0);
		return false;
	}*/

	//if win maps, a win is possible
	//if(p->x_wmap || p->opp_wmap)
	//	return false;

	//check for each player
	/*if(c4_player_can_win(p->x, p->filled))
		return false;
	if(c4_player_can_win(p->x ^ p->filled, p->filled))
		return false;*/

	/*printf("checking for impossible\n");
	window_unfocus();
	term_move_cursor(0,12);
	c4_draw_full(p, -1);
	getchar();*/

	//24 horiz paths (6 * 4)
	//12 up diag paths (3 * 4)
	//12 down diag paths (3 * 4)
	static uint64_t paths[48] = {0};
	if(!paths[0])
		make_paths(paths);

	uint64_t x = p->x;
	uint64_t opp = p->x ^ p->filled;
	for(int i=0; i<48; i++)
	{
		uint64_t path = paths[i];
		if(!((path & x) && (path & opp)))
			return false;
	}

	//test
	/*printf("pos w impossible win:\n");
	term_move_cursor(0, 12);
	c4_draw_full(p, -1);
	getchar();*/

	return true;
}

endstate_t c4_gameover(void *pos)
{
	assert(c4_ok(pos));
	c4_pos_t *p = pos;

	//uint64_t x = p->x ^ p->filled;

	int win = !c4_whosemove(p)? END_P1_WON : END_P2_WON;

	if(p->won)
		return win;

	//if(is_win(p->x))
	//	return win;

	//check draw
	//printf("filled = %s\n", print64(p->filled));
	uint64_t full = 0b0111111011111101111110111111011111101111110111111;
	//assert(p->filled <= full);
	//printf("good!\n");
	if((p->filled & ~WHOSEMOVE_BIT) == full)
	{
		//printf("draw detected!\n");
		//exit(0);
		return END_DRAW;
	}

	//if(c4_win_impossible(p))
	//	return END_DRAW;


	//
	return END_NOT_OVER;
}

float c4_estimate(void *pos)
{
	assert(c4_ok(pos));
	c4_pos_t *p = pos;

	uint64_t x = p->x | WHOSEMOVE_BIT;
	uint64_t opp = (p->x ^ p->filled) & ~WHOSEMOVE_BIT;


	//get_win_maps(p);
	//uint64_t x_wmap = win_map(x, p->filled);
	//uint64_t opp_wmap = win_map(opp, p->filled);
	get_win_maps(p);
	uint64_t x_wmap = p->x_wmap;
	uint64_t opp_wmap = p->opp_wmap;

	//if we can win on the next move, don't bother
	//with the other estimates
	//if((x_wmap>>1) & (p->filled | 0b100000010000001000000100000010000001000000))
	//	return c4_whosemove(p)? 500 : -500;


	//calculate est for each player
	float est = estimate_color(x, opp, p->filled, x_wmap, opp_wmap, false);
	est -= estimate_color(opp, x, p->filled, opp_wmap, x_wmap, false);

	//endgame analysis
	const int C4_ENDGAME_CT = 26;
	int move_ct = __builtin_popcount(p->filled & ~WHOSEMOVE_BIT);
	if(move_ct >= C4_ENDGAME_CT)
	{
		if(p->x_wmap && !p->opp_wmap)	est += 100;
		else if(p->opp_wmap && !p->x_wmap)	est -= 100;
	}

	if(!c4_whosemove(p))
		est *= -1;

	/*if(est >= 95 || est <= -95)
	{
		printf("parity win (%.1f):\n", est);
		c4_draw_full(pos);
		getchar();
	}*/

	///est /= 10;
	return est;
}


float estimate_sort_color(uint64_t x, uint64_t opp, uint64_t filled)
{
	float est = 0;
	/*

	//horizontal
	uint64_t three = x & (x<<7) & (x<<14);
	if(three)
	{
		if(three>>21 & ~opp
			& 0b0111111011111101111110111111011111101111110111111)
			est++;
		if(three<<7 & ~opp
			& 0b0111111011111101111110111111011111101111110111111)
			est++;
	}

	//fd diag
	three = (x>>8) & x & (x<<8);
	if(three)
	{
		if(three>>16 & ~opp
			& 0b0111111011111101111110111111011111101111110111111)
			est++;
		if(three<<16 & ~opp
			& 0b0111111011111101111110111111011111101111110111111)
			est++;
	}

	//bk diag
	three = (x>>6) & x & (x<<6);
	if(three)
	{
		if(three>>12 & ~opp
			& 0b0111111011111101111110111111011111101111110111111)
			est++;
		if(three<<12 & ~opp
			& 0b0111111011111101111110111111011111101111110111111)
			est++;
	}

	//vertical
	three = x & (x<<1) & (x<<2);
	if(three)
	{
		//if(!(three & opp<<3))	est++;
		if(three<<1 & ~opp)	est++;
	}*/

	//count all places where we're one move away from win
	uint64_t b = 0b1;
	int r = 0;
	for(int i=0; i<49; i++)
	{


		if(r == 6)
		{
			r = 0;
			b <<= 1;
			continue;
		}
		//printf("b=%s, r=%d\n", print64(b), r);

		if(filled & b)
		{
			r++;
			b<<=1;
			continue;
		}


		if(is_win(x | b))
		{
			est++;
			//if(verbose)
			//	printf("\twin sq @ c%d, r%d\n", i/7, i%7);
		}

		r++;
		b<<=1;
	}
	//printf("\n");


	return (float)est;
}

uint64_t win_map(uint64_t x, uint64_t filled)
{
	uint64_t wb;

	//vertical
	wb = (x<<1) & (x<<2) & (x<<3);

	uint8_t shifts[] = {6, 7, 8};
	for(int i=0; i<3; i++)
	{
		uint8_t sh = shifts[i];
		uint64_t p = (x<<(sh)) & (x<<(2*sh));
		wb |= p & (x<<(3*sh));
		wb |= p & (x>>(sh));
		p >>= 3*sh;
		wb |= p & (x<<(sh));
		wb |= p & (x>>(3*sh));
	}

	/*wb |= (x<<(6) & x<<(2*6) & x<<(3*6));
	wb |= (x>>(6) & x>>(2*6) & x>>(3*6));

	wb |= (x<<(7) & x<<(2*7) & x<<(3*7));
	wb |= (x>>(7) & x>>(2*7) & x>>(3*7));

	wb |= (x<<(8) & x<<(2*8) & x<<(3*8));
	wb |= (x>>(8) & x>>(2*8) & x>>(3*8));*/

	wb &= ~filled;

	//clear
	wb &= 0b0111111011111101111110111111011111101111110111111;
	//return __builtin_popcount(wb);
	return wb;
}

void get_win_maps(c4_pos_t *p)
{
	if(p->x_wmap == NO_WIN_MAP)
		p->x_wmap = win_map(p->x, p->filled);

	if(p->opp_wmap == NO_WIN_MAP)
		p->opp_wmap = win_map(p->x ^ p->filled, p->filled);

}

float c4_estimate_sort(void *pos, int move)
{
	c4_pos_t *p = pos;

	//int est = 0;

	uint64_t mb = move_bit(p, move);

	uint64_t filled = p->filled | mb;
	uint64_t x = p->x | mb;
	uint64_t opp = x ^ filled;

	float est = __builtin_popcount(win_map(x, filled));
	est -= __builtin_popcount(win_map(opp, filled));

	return est;

	//calculate est for the player whose turn it is
	//float est = estimate_sort_color(x, opp, filled);
	//est -= estimate_sort_color(opp, x, filled);

	/*
	//count all places where we're one move away from win
	uint64_t b = 0b1;
	int r = 0;
	for(int i=0; i<49; i++)
	{


		if(r == 6)
		{
			r = 0;
			b <<= 1;
			continue;
		}
		//printf("b=%s, r=%d\n", print64(b), r);

		if(filled & b)
		{
			r++;
			b<<=1;
			continue;
		}


		if(is_win(x | b))
			est++;
		if(is_win(opp | b))
			est--;

		r++;
		b<<=1;
	}

	///est /= 10;
	return (float)est;
	*/
}

float estimate_color_count_middles(uint64_t x)
{
	int est = 0;

	int scores[] = {0, 1, 2, 3, 2, 1, 0};
	for(int i=1; i<6; i++)
	{
		int col_bits = __builtin_popcount(get_col(x, i));
		est += scores[i] * col_bits;
	}
	return est;
}

float estimate_color_count_wins(uint64_t x, uint64_t filled,
	uint64_t wmap, bool verbose)
{
	//return __builtin_popcount(win_map(x, filled));
	return __builtin_popcount(wmap);

	/////////////////////////////////////

	//uint64_t wmap = win_map(x, filled);
	int wins = __builtin_popcount(wmap);

	//parity
	int nmoves = __builtin_popcount(filled & ~WHOSEMOVE_BIT);
	/*int nmoves = 0;
	for(int i=0; i<7; i++)
	{
		nmoves += __builtin_popcount(get_col(filled, i));
		//printf("\t0x%0x\n", get_col(filled, i));
	}*/
	bool parity = !(nmoves & 0b1);
	//printf("%d moves played (%s)\n", nmoves, parity? "even":"odd");
	//printf("filled: %s\n", print64(filled));

	if((x & WHOSEMOVE_BIT))
		parity = !parity;
	uint64_t pmask = 0b0010101001010100101010010101001010100101010010101;
	if(parity)
		pmask <<= 1;
	uint64_t parity_wins = wmap & pmask;

	//clear ones that can be immediately played
	//(win spots with a token already underneath them)
	parity_wins &= ~(filled<<1);

	if(parity_wins)
	{
		//assert(0);
		wins += 50;
	}

	return wins;
}

float estimate_color_count_open_threes(uint64_t x, uint64_t opp, bool verbose)
{
	int est = 0;

	//horizontal
	uint64_t three = x & (x<<7) & (x<<14);
	if(three)
	{
		if(three>>21 & ~opp
			& 0b0111111011111101111110111111011111101111110111111)
			est++;
		if(three<<7 & ~opp
			& 0b0111111011111101111110111111011111101111110111111)
			est++;
		if(verbose)
			printf("found horiz (est = %d)\n", est);
	}

	//fd diag
	three = (x>>8) & x & (x<<8);
	if(three)
	{
		if(three>>16 & ~opp
			& 0b0111111011111101111110111111011111101111110111111)
			est++;
		if(three<<16 & ~opp
			& 0b0111111011111101111110111111011111101111110111111)
			est++;
		if(verbose)
			printf("found fd diag (est = %d)\n", est);
	}

	//bk diag
	three = (x>>6) & x & (x<<6);
	if(three)
	{
		if(three>>12 & ~opp
			& 0b0111111011111101111110111111011111101111110111111)
			est++;
		if(three<<12 & ~opp
			& 0b0111111011111101111110111111011111101111110111111)
			est++;
		if(verbose)
			printf("found bk diag (est = %d)\n", est);
	}

	//vertical
	three = x & (x<<1) & (x<<2);
	if(three)
	{
		//if(!(three & opp<<3))	est++;
		if(three<<1 & ~opp)	est++;
		if(verbose)
			printf("found vert (est = %d)\n", est);
	}


	return (float)est;
}


float estimate_color(uint64_t x, uint64_t opp, uint64_t filled,
	uint64_t wmap, uint64_t opp_wmap, bool verbose)
//float estimate_color(uint64_t x, uint64_t filled, bool verbose)
{
	assert(wmap != NO_WIN_MAP);

	float est = 0;
	//int move_ct = __builtin_popcount(filled & ~WHOSEMOVE_BIT);
	//if(move_ct < 26)
		est += estimate_color_count_middles(x);
	est += 3*estimate_color_count_wins(x, filled, wmap, verbose);
	//est += 3*(__builtin_popcount(wmap));

	uint64_t stack = wmap & (wmap>>1);
	if(stack && !(stack & opp_wmap)
		 && !(stack>>1 & opp_wmap))
		est += 100;
	/*{
		window_unfocus();
		term_move_cursor(0, 12);
		c4_pos_t stack_p;
		stack_p.x = x;
		stack_p.filled = filled;
		c4_draw_full(&stack_p, -1);
		getchar();
	}*/

	//est += estimate_color_count_open_threes(x, opp, verbose);
	return est;
}



bool c4_is_legal(void *pos, int index)
{
	assert(c4_ok(pos));
	c4_pos_t *p = pos;

	uint8_t col = get_col(p->filled, index);

	return (col & 0b111111) != 0b111111;
}

void c4_make_move_temp(void *made, void *pos, int index, uint32_t *hash)
{
	c4_pos_t *p = pos;
	c4_pos_t *m = made;



	uint64_t col = get_col(p->filled, index)+1;
	uint64_t b = col<<(7*index);

	m->won = false;
	if(p->x_wmap != NO_WIN_MAP)
		if(p->x_wmap & b)
			m->won = true;
	//break;

	m->x = p->x ^ p->filled;
	m->filled = (p->filled | b) ^ WHOSEMOVE_BIT;

	//m->x_wmap = NO_WIN_MAP;
	m->x_wmap = (p->opp_wmap==NO_WIN_MAP)?
		NO_WIN_MAP : p->opp_wmap & ~b;
	m->opp_wmap = NO_WIN_MAP;

	//???
	/*if(p->x_wmap)
		m->won = p->opp_wmap & m->x;
	else
		m->won = false;*/

	//return &made;

	//update hash
	if(!hash)
		return;
	//assert(zobrist_computed);
	//*hash = check_hash;


	int hi = 6*index;

	for(uint8_t c=col>>1; c; c>>=1)
		hi++;
	//hi += 32 - __builtin_clz((unsigned int)col);
	//hi += __builtin_popcount(col);
	//printf("now hi is %d\n", hi);
	//if(p->whosemove)
	if(c4_whosemove(m))
		hi += 42;
	//printf("final hi: %d\n", hi);

	zobrist_place(hash, hi);

	//test
	//uint32_t check_hash = c4_hash(m, 0);
	//assert(*hash == check_hash);
}

int c4_get_placement(void *pos, int index)
{
	assert(index < 7);

	c4_pos_t *p = pos;

	uint64_t col = get_col(p->filled, index);
	//uint64_t b = col<<(7*index);

	int placement = __builtin_popcount(col);
	if(placement >= 6)
		return -1;
	placement += 6*index;
	assert(placement < 42);

	int player_place = placement;
	if(!c4_whosemove(p))
		player_place += 42;
	assert(player_place < 2*42);	//illegal

	//make sure the placement slot is empty
	uint64_t b = 1;
	for(int i=0; i<placement; i++)
	{
		b<<=1;
		if(!(i % 6))
			b<<=1;
	}
	assert(!(b & p->filled));

	return player_place;
}

void c4_make_move(void *pos, int index, uint32_t *hash)
{
	assert(c4_ok(pos));
	c4_pos_t *p = pos;



	//uint64_t b = p->filled + (((uint64_t)1) << (7*index));
	//b &= 0b0111111011111101111110111111011111101111110111111;
	//printf("0x%0x\n", b);

	uint64_t col = get_col(p->filled, index)+1;
	uint64_t b = col<<(7*index);

	p->won = false;
	if(p->x_wmap != NO_WIN_MAP)
		if(p->x_wmap & b)
		{
			p->won = true;
			//return;
		}

	//p->x |= b;
	//p->filled |= b;

	//invert x
	p->x ^= p->filled;

	p->filled |= b;

	//p->whosemove = !p->whosemove;
	p->filled ^= WHOSEMOVE_BIT;

	//p->opp_wmap = NO_WIN_MAP;
	p->x_wmap = (p->opp_wmap==NO_WIN_MAP)?
		NO_WIN_MAP : p->opp_wmap & ~b;
	p->opp_wmap = NO_WIN_MAP;

	//update hash
	if(!hash)
		return;
	//assert(zobrist_computed);
	//*hash = check_hash;


	int hi = 6*index;

	for(uint8_t c=col>>1; c; c>>=1)
		hi++;
	//hi += __builtin_popcount(col);
	//printf("now hi is %d\n", hi);
	//if(p->whosemove)
	if(c4_whosemove(p))
		hi += 42;
	//printf("final hi: %d\n", hi);

	zobrist_place(hash, hi);

	//test
	//uint32_t check_hash = c4_hash(pos, 0);
	//assert(*hash == check_hash);
}

bool c4_move_loses(void *pos, int move)
{
	//if(!c4_is_legal(pos, move))
	//	return false;

	c4_pos_t *p = pos;

	get_win_maps(p);


	uint64_t mb = move_bit(p, move);
	assert(mb);

	//a win move is never a losing move
	if(mb & p->x_wmap)
		return false;

	//don't play under an opponent's win
	return (mb<<1) & p->opp_wmap;
}

uint32_t c4_hash(void *key, size_t size)
{
	c4_pos_t *p = key;
	uint32_t h = 0;

	//printf("hash for pos:\nx=0x%0x\nfilled=0x%0x\n",
	//	p->x, p->filled);

	//generate bitstrings
	//if(!zobrist_computed)
	zobrist_init(85, ZOBRIST_SEED);

	//compute hash
	int r=0, i=0;
	uint64_t end = 0b1;
	end <<= 49;
	for(uint64_t b=0b1; b<end; b<<=1)
	{
		if(r == 6)	//skip bit btwn columns
		{
			r = 0;
			continue;
		}
		//printf("b=0x%016" PRIx64 ", i=%d, r=%d\n", b, i, r);
		if(p->filled & b)
		{
			//printf("filled bit %d\n", i);
			int index = i;
			if(p->x & b)
				index += (c4_whosemove(p))? 0 : 42;
			else
				index += (c4_whosemove(p))? 42 : 0;
			//printf("c4hash: zobrist w index %d\n", index);
			zobrist_place(&h, index);
		}

		i++;
		r++;
	}

	return h;
}

int c4_moves_remaining(void *pos)
{
	c4_pos_t *p = pos;
	int played = __builtin_popcount(p->filled);
	return (42-played)/2;
}

bool c4_keys_match(void *k1, void *k2)
{
	c4_pos_t *n1 = k1, *n2 = k2;

	if(n1->x != n2->x)
		return false;
	if(n1->filled != n2->filled)
		return false;
	//if(n1->whosemove != n2->whosemove)
	//	return false;

	return true;
}

void swap(uint8_t *a, uint8_t *b)
{
	uint8_t temp = *a;
	*a = *b;
	*b = temp;
}

uint64_t flip_64(uint64_t c)
{
	uint64_t flipped = c & (((uint64_t)1)<<63);
	for(int i=0; i<7; i++)
	{
		uint64_t col = get_col(c, i);
		flipped |= col << (7*(6-i));
	}
	//flipped &= 0b0111111011111101111110111111011111101111110111111;
	//flipped |= c & (((uint64_t)1)<<63);
	return flipped;
}

void c4_flip_horiz(void *to, void *from)
{
	c4_pos_t *f = from, *t = to;

	t->x = flip_64(f->x);
	t->filled = flip_64(f->filled);
}

int c4_make_movelist(sorter_t *sorter, void *pos)
{
	c4_pos_t *p = pos;
	int ct = 0;
	//int bad_move = -1;
	get_win_maps(p);
	for(int i=0; i<7; i++)
	{
		if(get_col(p->filled, i) != 0b111111)
		{
			sorter[ct].move = i;
			sorter[ct].score = 0;
			ct++;
		}
	}

	return ct;
}

int c4_only_moves(sorter_t *sorter, void *pos)
{
	c4_pos_t *p = pos;

	//get_win_maps(p);

	/*uint64_t move_map = 0;
	for(int i=0; i<7; i++)
	{
		uint64_t b = move_bit(p, i);
		move_map |= b;
	}*/

	uint64_t filled_alt = p->filled;
	filled_alt |= 0b100000010000001000000100000010000001000000;
	uint64_t move_map = (filled_alt<<1) | filled_alt;
	move_map ^= filled_alt;
	if(!(filled_alt & 0b1))
		move_map |= 0b1;

	/*if(move_map != other_move_map)
	{
		printf("filled:\t%s\n", sprintbig(p->filled, "%b"));
		printf("move map:\t%s\n", sprintbig(move_map, "%b"));
		printf("alt map:\t%s\n", sprintbig(other_move_map, "%b"));
		assert(0);
	}*/

	/*uint64_t alt_move_map = (p->filled<<1) & ~p->filled;
	if(alt_move_map != move_map)
	{
		printf("%s\n", print64(p->filled));
		printf("%s\n", print64(alt_move_map));
		printf("%s\n", print64(move_map));
		assert(0);
	}*/


	//get_win_map(&p->x_wmap, p->x, p->filled);
	//get_win_map(&p->opp_wmap, p->x ^ p->filled, p->filled);

	//uint64_t filled_1_higher = (p->filled<<1) | 0b000000100000010000001000000100000010000001;

	//check if we have a win on next move
	//uint64_t wmap = win_map(p->x, p->filled);
	//uint64_t wmap = p->x_wmap;
	if(p->x_wmap == NO_WIN_MAP)
		p->x_wmap = win_map(p->x, p->filled);
	uint64_t win_move = move_map & p->x_wmap;
	//uint64_t win_move = p->x_wmap & filled_1_higher;
	//if(move_map & p->x_wmap)
	if(win_move)
	{
		for(int i=0; i<7; i++)
		{
			win_move >>= 7;
			if(!win_move)
			{
				if(sorter)
					sorter[0].move = i;
				return 1;
			}
		}
		/*uint64_t col_mask = 0b111111;
		for(int i=0; i<7; i++)
		{
			//uint64_t mb = move_bit(p, i);
			//if(mb & p->x_wmap)
			if(col_mask & win_move)
			{
				if(sorter)
					sorter[0].move = i;
				return 1;
			}
			col_mask <<= 7;
		}*/
	}

	//check if opp has a win we have to block on next move
	//uint64_t opp = p->x ^ p->filled;
	//uint64_t opp_wmap = win_map(opp, p->filled);
	//uint64_t opp_wmap = p->opp_wmap;
	if(p->opp_wmap == NO_WIN_MAP)
		p->opp_wmap = win_map(p->x ^ p->filled, p->filled);
	//win_move = p->opp_wmap & filled_1_higher;
	win_move = move_map & p->opp_wmap;
	//if(move_map & p->opp_wmap)
	if(win_move)
	{
		for(int i=0; i<7; i++)
		{
			win_move >>= 7;
			if(!win_move)
			{
				if(sorter)
					sorter[0].move = i;
				return 1;
			}
		}
		/*uint64_t col_mask = 0b111111;
		for(int i=0; i<7; i++)
		{
			//uint64_t mb = move_bit(p, i);
			//if(mb & p->opp_wmap)
			if(col_mask & win_move)
			{
				if(sorter)
					sorter[0].move = i;
				return 1;
			}
			col_mask <<= 7;
		}*/
	}

	return 0;
}

/*void draw_color(uint8_t *cols, uint8_t *opp)
{
	char indent[] = "\t\t\t\t\t\t\t";

	//header
	printf(indent);
	for(int c=0; c<7; c++)
		printf("%d   ", c);


	for(int r=5; r>=0; r--)
	{
		printf("\n\n%s", indent);

		for(int col=0; col<7; col++)
		{
			char c = '_';
			if(cols[col] & (1<<r))
				c = 'x';
			printf("%c   ", c);
		}
	}
	putchar('\n');

	//estimate_color(cols, opp, true);
}*/

void c4_draw_full(void *pos, int last_move)
{
	c4_pos_t *p = pos;

	char indent[] = "\t\t\t\t\t\t\t    ";

	//test
	uint64_t red;//, yel;
	if(c4_whosemove(p))
	{
		//yel = p->x ^ p->filled;
		red = p->x;
	}
	else
	{
		red = p->x ^ p->filled;
		//yel = p->x;
	}

	uint64_t wb = 0;
	//if(p->won)
	{
		uint64_t x = p->x ^ p->filled;

		uint8_t shifts[] = {1, 6, 7, 8};
		for(int i=0; i<4; i++)
		{
			uint8_t sh = shifts[i];
			uint64_t mask = x & (x<<(sh)) & (x<<(2*sh)) & (x<<(3*sh));
			if(mask)
			{
				wb = mask | (mask>>(sh)) | (mask>>(2*sh)) | (mask>>(3*sh));
				break;
			}
		}
	}

	//uint64_t filled = p->filled;
	//uint64_t = ;
	get_win_maps(p);
	//get_win_map(&p->x_wmap, p->x, p->filled);
	//get_win_map(&p->opp_wmap, p->x ^ p->filled, p->filled);
	uint64_t rwm, ywm;
	(void)rwm;
	(void)ywm;
	if(c4_whosemove(p))
	{
		rwm = p->x_wmap;
		ywm = p->opp_wmap;
	}
	else
	{
		ywm = p->x_wmap;
		rwm = p->opp_wmap;
	}
	//uint64_t rwm = win_map(red, filled);
	//uint64_t ywm = win_map(yel, filled);

	uint64_t recent = 0;
	if(last_move != -1)
	{
		uint8_t rec_col = get_col(p->filled, last_move);
		for(uint8_t i=1; i<=0b1000000; i<<=1)
		{
			if(!(i & rec_col))
			{
				i>>=1;
				recent = i;
				recent <<= (7*last_move);
				break;
			}
		}
	}


	//header
	printf("\n\n\n\n\n");
	//printf(indent);
	//for(int c=0; c<7; c++)
	//	printf("%d   ", c);


	for(int r=5; r>=0; r--)
	{
		printf("\n\n%s", indent);
		uint64_t row_m = 1<<r;

		for(int col=0; col<7; col++)
		{
			char c = '_';
			char *color = TERM_WHITE;
			char *bg = TERM_BLACK_BG;
			uint64_t bit_m = row_m << (7*col);
			if(p->filled & bit_m)
			{
				//bool ry = (p->x & bit_m);
				//if(!c4_whosemove(p))
				//	ry = !ry;
				bool ry = red & bit_m;
				color = ry? TERM_RED : TERM_YELLOW;
				//c = ry? 'R':'Y';
				c = 'O';

				if(bit_m & recent)
					bg = TERM_BLUE_BG;
				else if(bit_m & wb)
					bg = TERM_WHITE_BG;

				//if(wm & bit_m)
				//	assert(0);
			}
			#ifdef SHOW_WIN_TILES
			else if((rwm | ywm) & bit_m)
			{
				c = '!';
				//color = 0;
				if(rwm & bit_m)	color = TERM_RED;
				if(ywm & bit_m)	color = TERM_YELLOW;
			}
			#endif
			//term_fg(color);
			printf("%s%s%c%s   ", bg, color, c, TERM_RESET);
			//term_clear();
		}
	}
	putchar('\n');

	/*
	//test
	for(int c=0; c<7; c++)
	{
		uint8_t col = 0b1111111 & (p->x >> (7*c));
		uint8_t filled = 0b1111111 & (p->filled >> (7*c));

		printf("col %d: x=0x%0x, filled=0x%0x\n",
			c, col, filled);
	}


	float est = estimate_color(p->x ^ p->filled, p->x, true);
	printf("est for current:\t%.0f\n", est);
	est = estimate_color(p->x, p->x ^ p->filled, true);
	printf("est for opp:\t\t%.0f\n", est);
	*/
	/*printf("est for current:\n");
	estimate_color(p->x ^ p->filled, p->x, true);
	printf("est for opp:\n");
	estimate_color(p->x, p->x ^ p->filled, true);*/

	//


}

void *c4_menu_define(void)
{
	menu_t *m = menu_grid(61, 17,	//x,y
		1, 7,	//rows,columns
		1, 4,	//r/c spacing
		"v\nv");

	menu_set(m, 3);
	return m;
}

int c4_menu_update(void *menu, void *pos, int key)
{
	if(key == '\n' || key == '\r')
		return menu_get(menu);

	menu_move_cursor(menu, key);
	return -1;
}

/*int c4_human_to_iter(char *human)
{
	if(human[0] >= 'a')
		human[0] -= ('a'-'A');
	int c = human[0]-'A';
	int r = human[1]-'0';
	int move = 3*r+c;
	return move;
}

char *c4_iter_to_human(int move)
{
	return NULL;
}*/



solver_t C4_SOLVER =
{
	.name = "connect four",

	.initial_pos = &C4_INIT_POS,
	.pos_size = sizeof(c4_pos_t),
	.hash_size = (uint8_t*)&C4_INIT_POS.x_wmap - (uint8_t*)&C4_INIT_POS,
	.possible_moves = 7,
	.possible_placements = 42*2,
	//.transtbl_buckets_ct = 180000001,
	.transtbl_buckets_ct = (1<<28),
	.iddfs_increment = 8,
	.aspiration_default_width = 1,
	.default_order = (uint8_t[]){2, 4, 6, 7, 5, 3, 1},
	.flip_depth = 20,

	.gameover = c4_gameover,
	.estimate = c4_estimate,
	.estimate_sort = c4_estimate_sort,
	.whosemove = c4_whosemove,
	.is_legal = c4_is_legal,
	.make_move = c4_make_move,
	.make_move_temp = c4_make_move_temp,
	.get_placement = c4_get_placement,
	.move_loses = c4_move_loses,
	.make_movelist = c4_make_movelist,
	.only_moves = c4_only_moves,

	.print_pos = NULL,
	.hash = c4_hash,
	.moves_remaining = c4_moves_remaining,
	.uses_zobrist = true,
	//.hash = NULL,
	.keys_match = c4_keys_match,
	.flip = c4_flip_horiz,

	.draw_full = c4_draw_full,
	.menu_define = c4_menu_define,
	.menu_update = c4_menu_update,
	.human_to_iter = NULL,
	.iter_to_human = NULL,
};
