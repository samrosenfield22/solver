

#include "c4.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

float estimate_color(uint8_t *c, uint8_t *opp, bool verbose);
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

	.whosemove = true,
	.columns_filled = {0, 0, 0, 0, 0, 0, 0},
	.columns_color = {0, 0, 0, 0, 0, 0, 0},
};

#define ZOBRIST_LEN	(85)

bool zobrist_computed = false;
uint32_t zobrist_strings[ZOBRIST_LEN];

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

void c4_zobrist_update(uint32_t *h, int c, int r, bool red_move)
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
}




//#define c4_ok(pos)	true
bool c4_ok(c4_pos_t *p)
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
}

endstate_t c4_gameover(void *pos)
{
	assert(c4_ok(pos));
	c4_pos_t *p = pos;

	/*
	get array of all reds, all yellows
		reds is just columns_color
		yellows is (~columns_color) & columns_filled
	call four_in_a_row(array)
	*/

	//check reds
	if(four_in_a_row(p->columns_color))
		return END_P1_WON;

	//check yellows
	uint8_t yellows[7];
	get_yellows(yellows, p);
	//for(int i=0; i<7; i++)
	//	yellows[i] = (~p->columns_color[i]) & p->columns_filled[i];
	if(four_in_a_row(yellows))
		return END_P2_WON;

	//check draw
	if((p->columns_filled[0]
		& p->columns_filled[1]
		& p->columns_filled[2]
		& p->columns_filled[3]
		& p->columns_filled[4]
		& p->columns_filled[5]
		& p->columns_filled[6]
	) == 0b111111)
		return true;
	/*bool all_filled = true;
	for(int i=0; i<7; i++)
	{
		if(p->columns_filled[i] != 0b111111)
		{
			all_filled = false;
			break;
		}
	}
	if(all_filled)
	{
		return END_DRAW;
	}*/

	//
	return END_NOT_OVER;
}

float c4_estimate(void *pos)
{
	assert(c4_ok(pos));
	c4_pos_t *p = pos;

	float est = 0;

	uint8_t yellows[7];
	get_yellows(yellows, p);
	est += (float)estimate_color(p->columns_color, yellows, false);
	est -= (float)estimate_color(yellows, p->columns_color, false);

	///est /= 10;
	return est;
}

float estimate_color(uint8_t *c, uint8_t *opp, bool verbose)
{

	//count all places where we're one move away from win
	/*float almost = 0;
	for(int i=0; i<7; i++)
	{
		for(uint8_t b=0b1; b<0b1000000; b<<=1)
		{
			if(!(c[i] & b))
			{
				c[i] |= b;
				if(four_in_a_row(c))
					almost++;
				c[i] &= ~b;
			}
		}
	}
	//return almost;
	*/





	int two_in_row_cnt = 0;
	int dir_patterns;

	//horiz
	dir_patterns = horiz_run_open(c, opp);
	if(verbose)
		printf("\t%d horiz patterns\n", dir_patterns);
	two_in_row_cnt += dir_patterns;

	//fd diag
	dir_patterns = fddiag_run_open(c, opp);
	if(verbose)
		printf("\t%d fd diag patterns\n", dir_patterns);
	two_in_row_cnt += dir_patterns;

	//bk diag
	dir_patterns = bkdiag_run_open(c, opp);
	if(verbose)
		printf("\t%d bk diag patterns\n", dir_patterns);
	two_in_row_cnt += dir_patterns;

	if(verbose)
		printf("two in row score = %d\n", two_in_row_cnt);

	return (float)two_in_row_cnt;
	//float score = two_in_row_cnt + almost;
	//return score;

	///////////////////////////



	/*
	//fd diag	/
	uint8_t cols_copy[7], opp_copy[7];
	for(int i=0; i<7; i++)
	{
		cols_copy[i] = c[i] >> (7-i);
		opp_copy[i] = opp[i] >> (7-i);
	}
	dir_patterns = two_horiz_open(cols_copy, opp_copy);
	if(verbose)
		printf("%d fd diag patterns\n", dir_patterns);
	two_in_row_cnt += dir_patterns;
	//two_in_row_cnt += two_horiz_open(cols_copy);

	//bk diag
	for(int i=0; i<7; i++)
	{
		cols_copy[i] = c[i] >> i;
		opp_copy[i] = opp[i] >> i;
	}
	dir_patterns = two_horiz_open(cols_copy, opp_copy);
	if(verbose)
		printf("%d bk diag patterns\n", dir_patterns);
	two_in_row_cnt += dir_patterns;
	//two_in_row_cnt += two_horiz_open(cols_copy);

	return two_in_row_cnt;
	*/

	/*float est = 0;
	float score = 1;
	for(int i=0; i<7; i++)
	{
		//count bits
		uint8_t col = c[i];
		int bits = 0;
		for(uint8_t b=1; b<0b1000000; b<<=1)
		{
			if(col & b)
				bits++;
		}

		//update est
		est += bits * score;

		if(i < 3)
			score++;
		else
			score--;
	}

	return est;*/
}

bool c4_is_legal(void *pos, int index)
{
	assert(c4_ok(pos));
	c4_pos_t *p = pos;

	return (p->columns_filled[index] != 0b111111);

	//int r, c;
	//index_to_rc(&r, &c, index);
	//return !(p->columns_filled[c] & (0b1 << r));
}

bool c4_whosemove(void *pos)
{
	assert(c4_ok(pos));
	c4_pos_t *p = pos;
	return p->whosemove;
}

void c4_make_move(void *pos, int index, uint32_t *hash)
{
	assert(c4_ok(pos));
	assert(zobrist_computed);
	c4_pos_t *p = pos;

	uint8_t *col = &p->columns_filled[index];
	uint8_t new_bit = *col;
	*col <<= 1;
	*col |= 1;
	new_bit ^= *col;


	if(p->whosemove)
		p->columns_color[index] |= new_bit;


	/*int r, c;
	index_to_rc(&r, &c, index);
	p->columns_filled[c] |= (0b1 << r);
	if(p->whosemove)
		p->columns_color[c] |= (0b1 << r);*/

	p->whosemove = !p->whosemove;

	//update hash
	if(!hash)
		return;
	int r = -1;
	for(uint8_t b=new_bit; b; b>>=1)	//count bits
		r++;
	//printf("new bit is 0x%0x, r=%d\n", new_bit, r);
	c4_zobrist_update(hash, index, r, !p->whosemove);
	c4_zobrist_update(hash, -1, 0, 0);	//update whosemove

	//test
	//uint32_t check_hash = c4_hash(pos, 0);
	//assert(*hash == check_hash);
}



uint32_t c4_hash(void *key, size_t size)
{
	c4_pos_t *p = key;
	uint32_t h = 0;

	//generate bitstrings
	if(!zobrist_computed)
	{
		zobrist_init();
	}

	//compute hash
	for(int c=0; c<7; c++)
	{
		for(int r=0; r<6; r++)
		{
			uint8_t b = (1<<r);
			if(p->columns_filled[c] & b)
			{
				int index = c * 6 + r;
				if(p->columns_color[c] & b)
					index += 42;

				zobrist_update(&h, index);
				//h ^= zobrist_strings[index];
			}
		}
	}

	if(p->whosemove)
		zobrist_update(&h, ZOBRIST_LEN-1);
		//h ^= zobrist_strings[84];

	//printf("hash = %u\n", h);
	return h;
}

/*uint32_t c4_hash(void *key, size_t size)
{

	c4_pos_t *p = key;
	uint64_t h = 0;
	uint8_t copy[7];
	uint32_t mult = 1;


	//compress
	for(int i=0; i<7; i++)
	{
		//add high bit to color byte
		uint8_t col = p->columns_color[i];
		uint8_t high = p->columns_filled[i] + 1;
		//printf("high = 0x%0x\n", high);
		col |= high;
		copy[i] = col;
		//printf("%x, ", copy[i]);

	}

	//calculate hash
	for(int i=0; i<7; i++)
	{
		h ^= copy[i];
		if(i != 6)
			h <<= 5;
		//mult *= 3;
	}
	if(p->whosemove)
		h++;
	h ^= h>>32;
	//printf("hash = %u\n", (uint32_t)h);
	return h;
}*/

bool c4_keys_match(void *k1, void *k2)
{
	c4_pos_t *n1 = k1, *n2 = k2;
	for(int i=0; i<7; i++)
	{
		if(n1->columns_filled[i] != n2->columns_filled[i])
			return false;

		if(n1->columns_color[i] != n2->columns_color[i])
			return false;
	}
	return n1->whosemove == n2->whosemove;
}

void swap(uint8_t *a, uint8_t *b)
{
	uint8_t temp = *a;
	*a = *b;
	*b = temp;
}

void c4_normalize(void *k)
{
	c4_pos_t *p = k;

	if(p->columns_filled[0] > p->columns_filled[6])
		for(int i=0; i<7; i++)
		{
			swap(&p->columns_filled[i], &p->columns_filled[6-i]);
			swap(&p->columns_color[i], &p->columns_color[6-i]);
		}
}

void draw_color(uint8_t *cols, uint8_t *opp)
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
}

void c4_draw_full(void *pos)
{
	c4_pos_t *p = pos;

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
			if(p->columns_filled[col] & (1<<r))
				c = (p->columns_color[col] & (1<<r)?
					'R':'Y');
			printf("%c   ", c);
		}
	}
	putchar('\n');

	//for(int i=0; i<7; i++)
	//	printf("%0x ", p->columns_filled[i]);

	/*
	//print individual colors for testing
	uint8_t yellows[7];
	get_yellows(yellows, p);
	printf("\n\n\t\t\t\t\t\t\t\treds:\n");
	draw_color(p->columns_color, yellows);

	printf("\n\n\t\t\t\t\t\t\t\tyellows:\n");
	draw_color(yellows, p->columns_color);

	printf("\nestimate = %f", c4_estimate(p));
	*/
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

bool four_in_a_row(uint8_t *cols)
{
	//vert
	for(int i=0; i<7; i++)
	{
		uint8_t c = cols[i];
		for(uint8_t m = 0b1111; m<0b1000000; m<<=1)
			if((m & c) == m)
				return true;
	}

	//horiz
	//uint16_t cols_16[7];
	//for(int i=0; i<7; i++)
	//	cols_16[i] = cols[i];
	if(four_horiz(cols))
		return true;

	if(four_fddiag(cols))
		return true;

	if(four_bkdiag(cols))
		return true;

	return false;

	/*
	//fd diag	/
	uint8_t cols_copy[7];
	for(int i=0; i<7; i++)
	{
		//cols_16[i] = cols[i];
		cols_copy[i] = cols[i] << (7-i);
	}
	if(four_horiz(cols_copy))
		return true;

	//bk diag
	for(int i=0; i<7; i++)
	{
		cols_copy[i] = cols[i] << i;
		//cols_16[i] <<= i;
	}
	if(four_horiz(cols_copy))
		return true;

	//no 4 in row
	return false;*/
}

/*int two_horiz_open(uint8_t *cols, uint8_t *opp)
{
	int cnt = 0;
	for(int i=1; i<=4; i++)
	{
		int opens = 0;
		uint8_t and = cols[i] & cols[i+1];
		//if(~cols[i-1] & cols[i] & cols[i+1] & ~cols[i+2])
		if(and)
		{
			if(and & ~cols[i-1] & ~opp[i-1])
				opens++;
			if(and & ~cols[i+2] & ~opp[i+2])
				opens++;
			if(opens == 2)
				opens++;	//3 for both sides open
			//cnt++;
			cnt += opens;
		}
	}
	return cnt;
}*/

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

		/*
		uint8_t and = cols[i] & cols[i+1]>>1;
		//if(~cols[i-1] & cols[i] & cols[i+1] & ~cols[i+2])
		if(and)
		{
			//check if left is open
			if(i)
				if(and & (~cols[i-1])<<1 & (~opp[i-1])<<1)
					opens++;
			if(i!=6)
				if(and & (~cols[i+2])>>2 & (~opp[i+2])>>2)
					opens++;
			if(opens == 2)
				opens++;	//3 for both sides open
			//cnt++;
			cnt += opens;
		}*/
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

		/*
		uint8_t and = cols[i] & cols[i+1]>>1;
		//if(~cols[i-1] & cols[i] & cols[i+1] & ~cols[i+2])
		if(and)
		{
			//check if left is open
			if(i)
				if(and & (~cols[i-1])<<1 & (~opp[i-1])<<1)
					opens++;
			if(i!=6)
				if(and & (~cols[i+2])>>2 & (~opp[i+2])>>2)
					opens++;
			if(opens == 2)
				opens++;	//3 for both sides open
			//cnt++;
			cnt += opens;
		}*/
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

		/*
		uint8_t and = cols[i] & cols[i+1]>>1;
		//if(~cols[i-1] & cols[i] & cols[i+1] & ~cols[i+2])
		if(and)
		{
			//check if left is open
			if(i)
				if(and & (~cols[i-1])<<1 & (~opp[i-1])<<1)
					opens++;
			if(i!=6)
				if(and & (~cols[i+2])>>2 & (~opp[i+2])>>2)
					opens++;
			if(opens == 2)
				opens++;	//3 for both sides open
			//cnt++;
			cnt += opens;
		}*/
	}
	return cnt;
}

/*int two_bkdiag_open(uint8_t *cols, uint8_t *opp)
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
}*/

bool four_horiz(uint8_t *cols)
{
	for(int i=0; i<4; i++)
	{
		if(cols[i] & cols[i+1] & cols[i+2] & cols[i+3])
			return true;
	}
	return false;

	/*for(int r=0; r<16; r++)
	{
		uint16_t m = (1<<r);
		int cnt = 0;
		for(int i=0; i<7; i++)
		{
			if(cols[i] & m)
				cnt++;
			else
				cnt = 0;

			if(cnt == 4)
				return true;
		}
	}

	return false;*/
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

//1 (old val) gets replaced with 2 (new val)
bool c4_replace_transpose(void *k_old, void *v_old,
	void *k_new, void *v_new)
{
	return true;

	trans_value_t *val_old = v_old;
	trans_value_t *val_new = v_new;

	if(val_new->iddfs > val_old->iddfs)
		return true;
	assert(val_old->iddfs == val_new->iddfs);

	if(val_old->depth > val_new->depth)
		return false;
	return true;

	c4_pos_t *n1 = k_old, *n2 = k_old;
	int turns_ish_1=0, turns_ish_2=0;
	for(int i=0; i<7; i++)
	{
		for(int b=0; ; b++)
		{
			if((1<<b) > n1->columns_filled[i])
			{
				turns_ish_1 += b;
				break;
			}
		}
		for(int b=0; ; b++)
		{
			if((1<<b) > n2->columns_filled[i])
			{
				turns_ish_2 += b;
				break;
			}
		}
		//turns_ish_1 += n1->columns_filled[i];
		//turns_ish_2 += n2->columns_filled[i];
	}

	return(turns_ish_1 > turns_ish_2);
}

solver_t C4_SOLVER =
{
	.name = "connect four",

	.initial_pos = &C4_INIT_POS,
	.pos_size = sizeof(c4_pos_t),
	.possible_moves = 7,
	.transtbl_buckets_ct = 40000003,
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
	//.normalize_position = c4_normalize,
	.normalize_position = NULL,
	.replace_transpose = c4_replace_transpose,

	.draw_full = c4_draw_full,
	.human_to_iter = NULL,
	.iter_to_human = NULL,
};
