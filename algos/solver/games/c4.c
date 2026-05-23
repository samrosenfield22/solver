

#include "c4.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <assert.h>

#include "../../../utils.h"

float estimate_color(uint64_t x, uint64_t opp, uint64_t filled, bool verbose);
void c4_draw_full(void *pos);
uint32_t c4_hash(void *key, size_t size);

#define WHOSEMOVE_BIT	(((uint64_t)1)<<63)

c4_pos_t C4_INIT_POS =
{
	.x = 0,
	.filled = WHOSEMOVE_BIT,
	//.whosemove = true,
};

#define ZOBRIST_LEN	(85)

bool zobrist_computed = false;
uint32_t zobrist_strings[ZOBRIST_LEN];

char *print64(uint64_t n)
{
	static char buf[64];
	snprintf(buf, 63, "0x%016" PRIx64 "", n);
	return buf;
}

void zobrist_init(void)
{
	assert(!zobrist_computed);
	zobrist_computed = true;

	/*tried srand seeds from 0 through 50
	best: 17 (43 collisions at d=18)
	worst: 19 (122)*/
	srand(9);
	for(int i=0; i<ZOBRIST_LEN; i++)
	{
		zobrist_strings[i] = rand()<<16 | rand();
	}
}

void zobrist_update(uint32_t *h, int n)
{
	*h ^= zobrist_strings[n];
}

/*void c4_zobrist_update(uint32_t *h, int c, int r, bool red_move)
{
	int index;
	if(c == -1)
		index = ZOBRIST_LEN-1;
	else
	{
		index = c * 6 + r;
		if(red_move)
			index += 42;
	}

	zobrist_update(h, index);
}*/

//#define c4_ok(pos)	true
/*bool c4_ok(c4_pos_t *p)
{
	if(!(p->whosemove==true || p->whosemove==false))
		return false;
	for(int i=0; i<7; i++)
	{
		if(p->columns_filled[i] & 0b11000000)
			return false;
		if(p->columns_color[i] & 0b11000000)
			return false;
	}
	return true;
}*/

bool c4_whosemove(void *pos)
{
	//assert(c4_ok(pos));
	c4_pos_t *p = pos;
	//return p->whosemove;
	return p->filled & WHOSEMOVE_BIT;
}

uint8_t get_col(uint64_t col, int index)
{
	uint8_t c = 0b111111;
	c &= col >> (7*index);
	return c;
}

//returns the bit required to make the given move (or with
//both x and filled)
uint64_t move_bit(c4_pos_t *p, int index)
{
	uint64_t col = get_col(p->filled, index)+1;
	return col<<(7*index);
}

bool is_win(uint64_t x)
{
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

endstate_t c4_gameover(void *pos)
{
	//assert(c4_ok(pos));
	c4_pos_t *p = pos;

	uint64_t x = p->x ^ p->filled;



	int win = !c4_whosemove(p)? END_P1_WON : END_P2_WON;

	if(is_win(x))
		return win;

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


	//
	return END_NOT_OVER;
}

float c4_estimate(void *pos)
{
	//assert(c4_ok(pos));
	c4_pos_t *p = pos;

	uint64_t x = p->x | WHOSEMOVE_BIT;
	uint64_t opp = (p->x ^ p->filled) & ~WHOSEMOVE_BIT;



	//calculate est for the player whose turn it is
	float est = estimate_color(x, opp, p->filled, false);
	est -= estimate_color(opp, x, p->filled, false);

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

float estimate_color_count_wins(uint64_t x, uint64_t filled, bool verbose)
{
	return __builtin_popcount(win_map(x, filled));

	/////////////////////////////////////

	uint64_t wmap = win_map(x, filled);
	int wins = __builtin_popcount(wmap);

	//parity
	int nmoves = 0;
	for(int i=0; i<7; i++)
	{
		nmoves += __builtin_popcount(get_col(filled, i));
		//printf("\t0x%0x\n", get_col(filled, i));
	}
	//int nmoves = __builtin_popcount(filled & ~WHOSEMOVE_BIT);
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
		wins += 20;
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


float estimate_color(uint64_t x, uint64_t opp, uint64_t filled, bool verbose)
//float estimate_color(uint64_t x, uint64_t filled, bool verbose)
{
	float est = 0;
	est += estimate_color_count_middles(x);
	est += 3*estimate_color_count_wins(x, filled, verbose);
	//est += estimate_color_count_open_threes(x, opp, verbose);
	return est;
}



bool c4_is_legal(void *pos, int index)
{
	//assert(c4_ok(pos));
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

	m->x = p->x ^ p->filled;
	m->filled = (p->filled | b) ^ WHOSEMOVE_BIT;

	//return &made;

	//update hash
	if(!hash)
		return;
	assert(zobrist_computed);
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

	zobrist_update(hash, hi);

	//test
	//uint32_t check_hash = c4_hash(m, 0);
	//assert(*hash == check_hash);
}

void c4_make_move(void *pos, int index, uint32_t *hash)
{
	//assert(c4_ok(pos));
	c4_pos_t *p = pos;

	//uint64_t b = p->filled + (((uint64_t)1) << (7*index));
	//b &= 0b0111111011111101111110111111011111101111110111111;
	//printf("0x%0x\n", b);

	uint64_t col = get_col(p->filled, index)+1;
	uint64_t b = col<<(7*index);


	//p->x |= b;
	//p->filled |= b;

	//invert x
	p->x ^= p->filled;

	p->filled |= b;

	//p->whosemove = !p->whosemove;
	p->filled ^= WHOSEMOVE_BIT;

	//update hash
	if(!hash)
		return;
	assert(zobrist_computed);
	//*hash = check_hash;


	int hi = 6*index;

	for(uint8_t c=col>>1; c; c>>=1)
		hi++;
	//hi += 32 - __builtin_clz((unsigned int)col);
	//hi += __builtin_popcount(col);
	//printf("now hi is %d\n", hi);
	//if(p->whosemove)
	if(c4_whosemove(p))
		hi += 42;
	//printf("final hi: %d\n", hi);

	zobrist_update(hash, hi);

	//test
	//uint32_t check_hash = c4_hash(pos, 0);
	//assert(*hash == check_hash);
}

bool c4_move_loses(void *pos, int move)
{
	if(!c4_is_legal(pos, move))
		return false;

	c4_pos_t *p = pos;
	c4_pos_t after = *p;
	c4_make_move(&after, move, NULL);

	bool losing = false;
	c4_pos_t next;
	for(int i=0; i<7; i++)
	{
		next = after;
		if(!c4_is_legal(&next, i))
			continue;

		c4_make_move(&next, i, NULL);
		int state = c4_gameover(&next);
		if(state==END_P1_WON || state==END_P2_WON)
		{
			losing = true;
			break;
		}
	}
	return losing;
}

uint32_t c4_hash(void *key, size_t size)
{
	c4_pos_t *p = key;
	uint32_t h = 0;

	//printf("hash for pos:\nx=0x%0x\nfilled=0x%0x\n",
	//	p->x, p->filled);

	//generate bitstrings
	if(!zobrist_computed)
		zobrist_init();

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
			zobrist_update(&h, index);
		}

		i++;
		r++;
	}

	return h;
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

int c4_only_move(void *pos)
{
	c4_pos_t *p = pos;

	//check if we have a win on next move
	uint64_t wmap = win_map(p->x, p->filled);
	for(int i=0; i<7; i++)
	{
		uint64_t mb = move_bit(p, i);
		if(mb & wmap)
			return i;
	}

	//check if opp has a win we have to block on next move
	uint64_t opp = p->x ^ p->filled;
	uint64_t opp_wmap = win_map(opp, p->filled);
	for(int i=0; i<7; i++)
	{
		uint64_t mb = move_bit(p, i);
		if(mb & opp_wmap)
			return i;
	}

	return -1;
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

void c4_draw_full(void *pos)
{
	c4_pos_t *p = pos;

	char indent[] = "\t\t\t\t\t\t\t";

	//test
	uint64_t red, yel;
	if(c4_whosemove(p))
	{
		yel = p->x ^ p->filled;
		red = p->x;
	}
	else
	{
		red = p->x ^ p->filled;
		yel = p->x;
	}

	uint64_t filled = p->filled;
	//uint64_t = ;
	uint64_t rwm = win_map(red, filled);
	uint64_t ywm = win_map(yel, filled);


	//header
	printf(indent);
	for(int c=0; c<7; c++)
		printf("%d   ", c);


	for(int r=5; r>=0; r--)
	{
		printf("\n\n%s", indent);
		uint64_t row_m = 1<<r;

		for(int col=0; col<7; col++)
		{
			char c = '_';
			int color = TERM_NEUTRAL;
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

				//if(wm & bit_m)
				//	assert(0);
			}
			else if((rwm | ywm) & bit_m)
			{
				c = '!';
				color = 0;
				if(rwm & bit_m)	color |= TERM_RED;
				if(ywm & bit_m)	color |= TERM_YELLOW;
			}
			term_fg(color);
			printf("%c   ", c);
			term_clear();
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



//1 (old val) gets replaced with 2 (new val)
bool c4_replace_transpose(void *k_old, void *v_old,
	void *k_new, void *v_new)
{
	return true;

	trans_value_t *val_old = v_old;
	trans_value_t *val_new = v_new;

	if((val_old->score >= MATE_LIMIT
		|| val_old->score <= MATE_LIMIT)
		&&
		!(val_new->score >= MATE_LIMIT
			|| val_new->score <= MATE_LIMIT))
		return false;
	int age = val_new->iddfs - val_old->iddfs;
	if(age >= 4)
		return true;
	//if(val_new->iddfs > val_old->iddfs)
	//	return true;
	//assert(val_old->iddfs == val_new->iddfs);

	if(val_old->depth+4 < val_new->depth)
		return false;
	return true;


}

solver_t C4_SOLVER =
{
	.name = "connect four",

	.initial_pos = &C4_INIT_POS,
	.pos_size = sizeof(c4_pos_t),
	.possible_moves = 7,
	//.transtbl_buckets_ct = 180000001,
	.transtbl_buckets_ct = (1<<28),
	.iddfs_increment = 4,
	.aspiration_default_width = 2,
	.default_order = (uint8_t[]){3, 2, 4, 1, 5, 0, 6},
	.flip_depth = 16,

	.gameover = c4_gameover,
	.estimate = c4_estimate,
	.estimate_sort = c4_estimate_sort,
	.whosemove = c4_whosemove,
	.is_legal = c4_is_legal,
	.make_move = c4_make_move,
	.make_move_temp = c4_make_move_temp,
	//.move_loses = c4_move_loses,
	.only_move = c4_only_move,

	.print_pos = NULL,
	.hash = c4_hash,
	.uses_zobrist = true,
	//.hash = NULL,
	.keys_match = c4_keys_match,
	//.keys_match = NULL,
	//.normalize_position = c4_normalize,
	.flip = c4_flip_horiz,
	//.normalize_position = NULL,
	.replace_transpose = c4_replace_transpose,

	.draw_full = c4_draw_full,
	.human_to_iter = NULL,
	.iter_to_human = NULL,
};
