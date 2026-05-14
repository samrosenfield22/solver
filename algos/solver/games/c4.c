

#include "c4.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <assert.h>

float estimate_color(uint64_t x, uint64_t opp, bool verbose);
bool four_in_a_row(uint8_t *cols);
bool four_horiz(uint8_t *cols);
bool four_fddiag(uint8_t *cols);
bool four_bkdiag(uint8_t *cols);
int horiz_run_open(uint8_t *cols, uint8_t *opp);
int fddiag_run_open(uint8_t *cols, uint8_t *opp);
int bkdiag_run_open(uint8_t *cols, uint8_t *opp);
void get_yellows(uint8_t *yellows, c4_pos_t *p);

uint32_t c4_hash(void *key, size_t size);

c4_pos_t C4_INIT_POS =
{
	.x = 0,
	.filled = 0,
	.whosemove = true,
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

uint8_t get_col(uint64_t col, int index)
{
	uint8_t c = 0b1111111;
	c &= col >> (7*index);
	return c;
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



	int win = !p->whosemove? END_P1_WON : END_P2_WON;

	if(is_win(x))
		return win;

	//check draw
	//printf("filled = %s\n", print64(p->filled));
	uint64_t full = 0b0111111011111101111110111111011111101111110111111;
	assert(p->filled <= full);
	//printf("good!\n");
	if(p->filled == full)
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


	//float est = 0;

	uint64_t x = p->x;
	uint64_t opp = p->x ^ p->filled;

	//float est = estimate_color(me, opp, false);
	//est -= estimate_color(opp, me, false);

	//calculate est for the player whose turn it is
	float est = estimate_color(x, opp, false);
	est -= estimate_color(opp, x, false);

	if(!p->whosemove)
		est *= -1;

	///est /= 10;
	return est;
}

float estimate_color(uint64_t x, uint64_t opp, bool verbose)
//float estimate_color(uint64_t x, uint64_t filled, bool verbose)
{
	int est = 0;

	int scores[] = {1, 2, 3, 5, 3, 2, 1};
	for(int i=0; i<7; i++)
	{
		int col_bits = __builtin_popcount(get_col(x, i));
		est += scores[i] * col_bits;
	}
	return est;

	/*
	//count all places where we're one move away from win
	uint64_t b = 0b1;
	int r = -1;
	for(int i=0; i<49; i++)
	{
		r++;
		if(r == 6)
		{
			r = 0;
			b <<= 1;
			continue;
		}
		if(filled & b)
		{
			b<<=1;
			continue;
		}


		if(is_win(x | b))
		{
			est++;
			if(verbose)
				printf("\twin sq @ c%d, r%d\n", i/7, i%7);
		}

		b<<=1;
	}
	if(verbose)
		printf("\twin sqs = %d\n", est);
	return (float)est;
	*/

	//horizontal
	uint64_t three = x & (x<<7) & (x<<14);
	if(three)
	{
		if(three>>21 & ~opp
			& 0b011111101111110111111011111101111110111111)
			est++;
		if(three<<7 & ~opp
			& 0b011111101111110111111011111101111110111111)
			est++;
		if(verbose)
			printf("found horiz (est = %d)\n", est);
	}

	//fd diag
	three = (x>>8) & x & (x<<8);
	if(three)
	{
		if(three>>16 & ~opp
			& 0b011111101111110111111011111101111110111111)
			est++;
		if(three<<16 & ~opp
			& 0b011111101111110111111011111101111110111111)
			est++;
		if(verbose)
			printf("found fd diag (est = %d)\n", est);
	}

	//bk diag
	three = (x>>6) & x & (x<<6);
	if(three)
	{
		if(three>>12 & ~opp
			& 0b011111101111110111111011111101111110111111)
			est++;
		if(three<<12 & ~opp
			& 0b011111101111110111111011111101111110111111)
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



bool c4_is_legal(void *pos, int index)
{
	//assert(c4_ok(pos));
	c4_pos_t *p = pos;

	uint8_t col = get_col(p->filled, index);

	return (col & 0b111111) != 0b111111;
}

bool c4_whosemove(void *pos)
{
	//assert(c4_ok(pos));
	c4_pos_t *p = pos;
	return p->whosemove;
}

void c4_make_move(void *pos, int index, uint32_t *hash)
{
	//assert(c4_ok(pos));
	c4_pos_t *p = pos;

	//uint64_t b = p->filled + (((uint64_t)1) << (7*index));
	//b &= 0b011111101111110111111011111101111110111111;
	//printf("0x%0x\n", b);

	uint64_t col = get_col(p->filled, index)+1;
	//col &= 0b111111;
	uint64_t b = col<<(7*index);


	p->x |= b;
	p->filled |= b;

	//invert x
	p->x ^= p->filled;

	p->whosemove = !p->whosemove;

	//update hash
	if(!hash)
		return;
	assert(zobrist_computed);
	//*hash = check_hash;


	int hi = 6*index;

	for(uint8_t c=col; c; c>>=1)
		hi++;
	//hi += 32 - __builtin_clz((unsigned int)col);
	//hi += __builtin_popcount(col);
	//printf("now hi is %d\n", hi);
	if(p->whosemove)
		hi += 42;
	//printf("final hi: %d\n", hi);

	zobrist_update(hash, hi);

	//test
	//uint32_t check_hash = c4_hash(pos, 0);
	//assert(*hash == check_hash);
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
				index += (p->whosemove)? 0 : 42;
			else
				index += (p->whosemove)? 42 : 0;
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

/*void c4_normalize(void *k)
{
	c4_pos_t *p = k;

	if(p->columns_filled[0] > p->columns_filled[6])
		for(int i=0; i<7; i++)
		{
			swap(&p->columns_filled[i], &p->columns_filled[6-i]);
			swap(&p->columns_color[i], &p->columns_color[6-i]);
		}
}*/

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

	estimate_color(cols, opp, true);
}*/

void c4_draw_full(void *pos)
{
	printf("drawing...\n");

	c4_pos_t *p = pos;

	char indent[] = "\t\t\t\t\t\t\t";

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
			uint64_t bit_m = row_m << (7*col);
			if(p->filled & bit_m)
			{
				bool ry = (p->x & bit_m);
				if(!p->whosemove)
					ry = !ry;
				c = ry? 'R':'Y';

			}
			printf("%c   ", c);
		}
	}
	putchar('\n');

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

	/*printf("est for current:\n");
	estimate_color(p->x ^ p->filled, p->x, true);
	printf("est for opp:\n");
	estimate_color(p->x, p->x ^ p->filled, true);*/
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


/*

int horiz_run_open(uint8_t *cols, uint8_t *opp)
{
	int cnt = 0;
	for(int i=0; i<6; i++)
	{
		int opens = 0;
		int len = 0;
		uint8_t and = 0xFF;

		for(int j=0; j<3; j++)
		{
			if(i+j >= 7)
				break;
			if(and & cols[i+j])
			{
				and &= cols[i+j];
				len++;
			}
			else
				break;
		}

		if(len < 2)
			continue;

		//check if l is open
		if(i)
			//if(cols[i] & (~cols[i-1]) & (~opp[i-1]))
			//	opens++;
			opens += __builtin_popcount(and
				& (~opp[i-1]));

		//check if r is open
		if(i+len<7)
			opens += __builtin_popcount(and
				& (~opp[i+len]));
			//if(cols[i+len-1]
			//	& (~cols[i+len]) & (~opp[i+len]))
			//	opens++;

		//if(opens == 2)	opens++;
		cnt += opens * ((len==2)? 1 : 5);
		//i+=len;


	}
	return cnt;
}

int fddiag_run_open(uint8_t *cols, uint8_t *opp)
{
	int cnt = 0;
	for(int i=0; i<6; i++)
	{
		int opens = 0;

		uint8_t and = 0xFF;
		int len = 0;
		for(int j=0; j<3; j++)
		{
			if(i+j >= 7)
				break;

			if(and & cols[i+j]>>j)
			{
				and &= cols[i+j]>>j;
				len++;
			}
			else
				break;
		}

		if(len < 2)
			continue;

		//check if l is open
		if(i)
			opens += __builtin_popcount(and
				& ~(opp[i-1])<<1);
				//& (~cols[i-1])<<1 & (~opp[i-1])<<1);
			//if(cols[i] & (~cols[i-1])<<1 & (~opp[i-1])<<1)
			//	opens++;

		//check if r is open
		if(i+len<7)
			opens += __builtin_popcount(and
				& ~(opp[i+len])>>len);
				//& (~cols[i+len])>>len & (~opp[i+len])>>len);
			//if(cols[i+len-1]
			//	& (~cols[i+len])>>(i+len) & (~opp[i+len])>>(i+len))
			//	opens++;

		//if(opens == 2)	opens++;
		cnt += opens * ((len==2)? 1 : 5);
		//i+=len;


	}
	return cnt;
}

int bkdiag_run_open(uint8_t *cols, uint8_t *opp)
{
	int cnt = 0;
	for(int i=0; i<6; i++)
	{
		int opens = 0;

		uint8_t and = 0xFF;
		int len = 0;
		for(int j=0; j<3; j++)
		{
			if(i+j >= 7)
				break;

			if(and & cols[i+j]<<j)
			{
				and &= cols[i+j]<<j;
				len++;
			}
			else
				break;
			and &= cols[i+j]<<j;
		}

		if(len < 2)
			continue;

		//check if l is open
		if(i)
			opens += __builtin_popcount(and
				& ~(opp[i-1])>>1);
				//(~cols[i-1])>>1 & (~opp[i-1])>>1);
			//if(cols[i] & (~cols[i-1])>>1 & (~opp[i-1])>>1)
			//	opens++;

		//check if r is open
		if(i+len<7)
			opens += __builtin_popcount(and
				& (uint8_t)(~opp[i+len])<<len);
				//& (~cols[i+len])<<len & (~opp[i+len])<<len);
			//if(cols[i+len-1]
			//	& (~cols[i+len])<<(i+len) & (~opp[i+len])<<(i+len))
			//	opens++;

		//if(opens == 2)	opens++;
		cnt += opens * ((len==2)? 1 : 5);
		//i+=len;


	}
	return cnt;
}

int two_bkdiag_open(uint8_t *cols, uint8_t *opp)
{
	int cnt = 0;
	for(int i=0; i<6; i++)
	{
		int opens = 0;
		uint8_t and = cols[i]>>1 & cols[i+1];
		//if(~cols[i-1] & cols[i] & cols[i+1] & ~cols[i+2])
		if(and)
		{
			//check if left is open
			if(i)
				if(and & (~cols[i-1])>>2 & (~opp[i-1])<<2)
					opens++;
			//check if right is open
			if(i!=6)
				if(and & (~cols[i+2])<<1 & (~opp[i+2])<<1)
					opens++;
			if(opens == 2)
				opens++;	//3 for both sides open
			//cnt++;
			cnt += opens;
		}
	}
	return cnt;
}

bool four_horiz(uint8_t *cols)
{
	for(int i=0; i<4; i++)
	{
		if(cols[i] & cols[i+1] & cols[i+2] & cols[i+3])
			return true;
	}
	return false;


}

bool four_fddiag(uint8_t *cols)
{
	for(int i=0; i<4; i++)
	{
		if(cols[i] & cols[i+1]>>1
			& cols[i+2]>>2 & cols[i+3]>>3)
			return true;
	}
	return false;
}

bool four_bkdiag(uint8_t *cols)
{
	for(int i=0; i<4; i++)
	{
		if(cols[i]>>3 & cols[i+1]>>2
			& cols[i+2]>>1 & cols[i+3]>>0)
			return true;
	}
	return false;
}

void get_yellows(uint8_t *yellows, c4_pos_t *p)
{
	for(int i=0; i<7; i++)
		yellows[i] = (~(p->columns_color[i])) & p->columns_filled[i];
}
*/

//1 (old val) gets replaced with 2 (new val)
bool c4_replace_transpose(void *k_old, void *v_old,
	void *k_new, void *v_new)
{
	//return true;

	trans_value_t *val_old = v_old;
	trans_value_t *val_new = v_new;

	if(val_new->iddfs > val_old->iddfs)
		return true;
	assert(val_old->iddfs == val_new->iddfs);

	if(val_old->depth > val_new->depth)
		return false;
	return true;


}

solver_t C4_SOLVER =
{
	.name = "connect four",

	.initial_pos = &C4_INIT_POS,
	.pos_size = sizeof(c4_pos_t),
	.possible_moves = 7,
	.transtbl_buckets_ct = 130000003,
	.default_order = (uint8_t[]){3, 2, 4, 1, 5, 0, 6},

	.gameover = c4_gameover,
	.estimate = c4_estimate,
	.whosemove = c4_whosemove,
	.is_legal = c4_is_legal,
	.make_move = c4_make_move,

	.print_pos = NULL,
	.hash = c4_hash,
	.uses_zobrist = true,
	//.hash = NULL,
	.keys_match = c4_keys_match,
	//.keys_match = NULL,
	//.normalize_position = c4_normalize,
	//.normalize_position = NULL,
	.replace_transpose = c4_replace_transpose,

	.draw_full = c4_draw_full,
	.human_to_iter = NULL,
	.iter_to_human = NULL,
};
