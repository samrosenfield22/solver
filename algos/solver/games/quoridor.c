/*board is 9x9
8x8 vertices where gates can be placed -> 2 uint64_ts
	(horiz, vert)
can use __int128 for pieces


need aux structure for pathfinding (only changes when gates placed)
*/

#include "quoridor.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <assert.h>

#include "../../../utils.h"

#define TOKEN_MOVES	(4)
#define HORIZ_PLACEMENTS	(72 + TOKEN_MOVES)

#define QUOR_Z_LEN	(2*81+2*72+1)

quor_pos_t QUOR_INIT_POS =
{
	.horiz=0, .vert=0, .gates=0,
	.p1=
	{
		.x=4, .y=0,
		.gate_ct=10,
		.token=((__int128)1)<<4
	},
	.p2=
	{
		.x=4, .y=8,
		.gate_ct=10,
		.token=((__int128)1)<<76
	},
	.whosemove = true,
};

enum
{
	MOVE_UP		= 0,
	MOVE_DOWN	= 1,
	MOVE_LEFT	= 2,
	MOVE_RIGHT	= 3,
};


//
void quor_draw_full(void *pos);
uint32_t quor_hash(void *key, size_t size);



quor_player_t *current(quor_pos_t *p)
{
	return (p->whosemove)? &p->p1 : &p->p2;
}

quor_player_t *current_opp(quor_pos_t *p)
{
	return (p->whosemove)? &p->p2 : &p->p1;
}

bool quor_ok(quor_pos_t *p)
{
	assert(p->p1.token);
	assert(p->p2.token);
	//assert(p->p1.token != p->p2.token);
	if(p->p1.token == p->p2.token)
	{
		quor_draw_full(p);
		printf("tokens equal!\n");
		assert(0);
	}

	assert(p->p1.x < 9);
	assert(p->p1.y < 9);
	assert(p->p2.x < 9);
	assert(p->p2.y < 9);


	//x/y and bitboard have to agree
	quor_player_t *me = &p->p1;
	int square = me->y*9 + me->x;
	__int128 bb = 1;
	bb <<= square;
	if(bb != me->token)
	{
		printf("on [%d,%d] (square %d):\n", me->x, me->y,
			me->y*9 + me->x);
		printbig(bb, "%x");
		printf("\n");
		printbig(me->token, "%x");

	}
	assert(bb == me->token);

	me = &p->p2;
	square = me->y*9 + me->x;
	bb = 1;
	bb <<= square;
	assert(bb == me->token);

	//no crisscrossing gates
	/*assert(!(p->horiz & p->vert));
	assert(!(p->horiz & p->horiz<<1));
	assert(!(p->vert & p->vert<<9));
	*/

	return true;
}


endstate_t quor_gameover(void *pos)
{
	assert(quor_ok(pos));
	quor_pos_t *p = pos;
	//quor_player_t *me = current(p);

	//int win = !quor_whosemove(p)? END_P1_WON : END_P2_WON;

	if(p->whosemove)
	{
		//if(p->opp_token < ((__int128(1))<<9)
		if(p->p2.y == 0)
			return END_P2_WON;
	}
	else
	{
		//if(p->me_token >= ((__int128(1))<<72)
		if(p->p1.y == 8)
			return END_P1_WON;
	}

	//no draws
	return END_NOT_OVER;
}

__int128 index_get_gate_bit(int index)
{
	int gate_index = index - TOKEN_MOVES;
	if(gate_index >= 72)
		gate_index -= 72;
	assert(gate_index < 72);
	__int128 gate_bit = 1;
	gate_bit <<= gate_index;
	return gate_bit;
}

bool quor_whosemove(void *pos)
{
	quor_pos_t *p = pos;
	return p->whosemove;
}

void move_player_token(quor_player_t *me, int x, int y, int dist)
{
	me->x += x;
	me->y += y;
	if(dist > 0)
		me->token <<= dist;
	else
		me->token >>= (0-dist);
}

bool quor_is_legal(void *pos, int index)
{
	//assert(quor_ok(pos));
	quor_pos_t *p = pos;
	quor_player_t *me = current(p), *opp = current_opp(p);

	int dist_x=0, dist_y = 0;

	//make token double bitmaps (vert + horiz)

	//__int128 *me = p->whosemove?
	//	&(p->me_token) : &(p->opp_token);
	//uint8_t player_x = p->whosemove? p->me_x : p->opp_x;

	if(index < TOKEN_MOVES)
	{
		switch(index)
		{
			case MOVE_UP:
				if(me->token & p->horiz)
					//|| me->token & p->horiz<<1)
					return false;
				if(me->token > ((__int128)1)<<71)
					return false;
				dist_y++;
				break;

			case MOVE_DOWN:
				if(me->token & p->horiz<<9)
					//|| me->token & p->horiz<<10)
					return false;
				if(me->token < ((__int128)1)<<9)
					return false;
				dist_y--;
				break;

			case MOVE_RIGHT:
				if(me->token & p->vert)
					//|| me->token & p->vert>>9)
					return false;
				if(me->x == 8)
					return false;
				dist_x++;
				break;

			case MOVE_LEFT:
				if(me->token & p->vert<<1)
					//|| me->token & p->vert>>8)
					return false;
				if(me->x == 0)
					return false;
				dist_x--;
				break;
		}

		if(me->x+dist_x == opp->x && me->y+dist_y == opp->y)
		{
			uint8_t saved_x=me->x, saved_y=me->y;
			__int128 saved_token=me->token;

			int dist = 9*dist_y + dist_x;
			move_player_token(me, dist_x, dist_y, dist);
			bool jump_legal = quor_is_legal(pos, index);

			//restore
			me->x = saved_x;
			me->y = saved_y;
			me->token = saved_token;

			return jump_legal;
		}
		//	return false;	//for now (should attempt hop)
	}
	else	//placing a gate
	{
		if(me->gate_ct == 0)
			return false;

		//no gates hanging off the edge
		if(((index-4) % 9)==8)
			return false;


		__int128 gate_bit = index_get_gate_bit(index);
		//if(gate_bit & (p->horiz | p->vert))
		if(gate_bit & p->gates)
			return false;

		//check overlapping w same kind
		if(index < HORIZ_PLACEMENTS)
		{
			if(gate_bit & (p->horiz | p->horiz>>1))
				return false;
		}
		else	//vert
		{
			if(gate_bit & (p->vert | p->vert>>9))
				return false;
		}
	}

	return true;
}

void quor_make_move(void *pos, int index, uint32_t *hash)
{
	//assert(quor_ok(pos));
	quor_pos_t *p = pos;
	quor_player_t *me = current(p);
	quor_player_t *opp = current_opp(p);

	int token_before, token_after;

	//make the move
	if(index < TOKEN_MOVES)
	{
		token_before = me->y*9 + me->x;
		if(!quor_whosemove(p))
			token_before += 81;
		token_after = token_before;

		//printf("moving from %d,%d w index %d\n",
		//	me->x, me->y, index);
		int move_x=0, move_y=0;
		switch(index)
		{
			case MOVE_UP:
				move_y = 1;
				/*me->y++;
				token_after += 9;
				me->token <<= 9;*/
				break;
			case MOVE_DOWN:
				move_y = -1;
				/*me->y--;
				token_after -= 9;
				me->token >>= 9;*/
				break;
			case MOVE_RIGHT:
				move_x = 1;
				/*me->x++;
				token_after++;
				me->token <<= 1;*/
				break;
			case MOVE_LEFT:
				move_x = -1;
				/*me->x--;
				token_after--;
				me->token >>= 1;*/
				break;

			default: assert(0);
		}

		int d = 9*move_y + move_x;
		move_player_token(me, move_x, move_y, d);
		token_after += d;
		if(me->token == opp->token)
		{
			move_player_token(me, move_x, move_y, d);
			token_after += d;
		}

		//printf("went to %d,%d\n",
		//	me->x, me->y);
	}
	else
	{
		me->gate_ct--;
		__int128 gb = index_get_gate_bit(index);
		p->gates |= gb;
		if(index < HORIZ_PLACEMENTS)
		{
			//printbig(gb, "%x");
			//printbig(gb<<1, "%x");
			//printbig(p->horiz, "%x");

			__int128 temp __attribute__((aligned(16))) = (gb | gb<<1);
			p->horiz |= temp;
			//p->horiz |= (gb | gb<<1);
		}
		else
		{
			__int128 temp __attribute__((aligned(16))) = (gb | gb<<9);
			p->vert |= temp;
		}
	}

	p->whosemove = !p->whosemove;

	//update hash
	if(!hash)
		return;


	if(index < TOKEN_MOVES)
	{
		zobrist_move(hash, token_after, token_before);
	}
	else
	{
		int g_index = index - TOKEN_MOVES + 2*81;
		zobrist_place(hash, g_index);
	}

	//test
	//uint32_t check_hash = quor_hash(pos, 0);
	//assert(*hash == check_hash);
}

float estimate_player(quor_player_t *pl, bool whosemove)
{
	const float CLOSE_TO_EXIT_SCORE = 1.0;
	const float GATES_LEFT_SCORE = 0.5;

	float est = 0;

	float close = whosemove? pl->y : 9-pl->y;
	est += CLOSE_TO_EXIT_SCORE * close;

	est += GATES_LEFT_SCORE * pl->gate_ct;

	return est;
}

float quor_estimate(void *pos)
{
	quor_pos_t *p = pos;

	float est = estimate_player(&p->p1, true);
	est -= estimate_player(&p->p2, false);

	return est;
}

uint32_t quor_hash(void *key, size_t size)
{
	quor_pos_t *p = key;
	uint32_t h = 0;
	zobrist_init(QUOR_Z_LEN, 0);

	zobrist_place(&h, p->p1.y*9 + p->p1.x);
	zobrist_place(&h, 81 + p->p2.y*9 + p->p2.x);


	int i=0;
	for(__int128 b=1; b; b<<=1)
	{
		if(p->horiz & b)
		{
			//printf("horiz @ index %d\n", i + 2*81);
			zobrist_place(&h, i + 2*81);
		}
		else if(p->vert & b)
		{
			//printf("vert @ index %d\n", i + 2*81+72);
			zobrist_place(&h, i + 2*81+72);
		}

		i++;
		//if(i > 72)
		//	break;
	}

	return h;
}

bool quor_keys_match(void *k1, void *k2)
{
	quor_pos_t *pos1 = k1, *pos2 = k2;
	if(pos1->horiz != pos2->horiz)	return false;
	if(pos1->vert != pos2->vert)	return false;
	if(pos1->gates != pos2->gates)	return false;
	if(pos1->p1.token != pos2->p1.token)	return false;
	if(pos1->p2.token != pos2->p2.token)	return false;
	if(pos1->p1.gate_ct != pos2->p1.gate_ct)	return false;
	if(pos1->p2.gate_ct != pos2->p2.gate_ct)	return false;

	return true;
}

/*void swap(uint8_t *a, uint8_t *b)
{
	uint8_t temp = *a;
	*a = *b;
	*b = temp;
}*/

void flip_128(__int128 *to, __int128 *from)
{
	*to = 0;
	__int128 end = 1;
	end <<= 81;
	int c = 0;
	for(__int128 b=1; b<end; b<<=1)
	{
		if(*from & b)
		{
			__int128 rev = b;
			if(c < 4)
				rev <<= 2*(4-c);
			else
				rev >>= 2*(c-4);
			*to |= rev;
		}

		c++;
		if(c == 9)
			c = 0;
	}
}

void flip_player(quor_player_t *to, quor_player_t *from)
{
	//x, y, gate_ct, token
	to->x = 8 - from->x;
	to->y = from->y;
	to->gate_ct = from->gate_ct;
	to->token = 1;
	to->token <<= to->y*9 + to->x;
}

void quor_flip(void *to, void *from)
{
	quor_pos_t *pt = to, *pf = from;


	flip_128(&pt->horiz, &pf->horiz);
	flip_128(&pt->vert, &pf->vert);
	flip_128(&pt->gates, &pf->gates);
	pt->whosemove = pf->whosemove;

	flip_player(&pt->p1, &pf->p1);
	flip_player(&pt->p2, &pf->p2);

	assert(quor_ok(to));
}

/*int quor_only_moves(sorter_t *sorter, void *pos)
{
	quor_pos_t *p = pos;
	//get win
	//get saving moves
	return 0;
}*/

void print_player_info(quor_player_t *pl)
{
	printf("player @ %d,%d (", pl->x, pl->y);
	printbig(pl->token, "%x");
	printf(")\n");
}

void quor_draw_full(void *pos)
{
	quor_pos_t *p = pos;

	char indent[] = "\t\t\t\t\t\t\t";

	__int128 b = 1;
	b <<= 72;



	const int height = 2;
	char c;
	for(int y=8; y>=0; y--)
	{
		for(int line=0; line<height; line++)
		{
			printf("\n%s    ", indent);

			for(int x=0; x<9; x++)
			{
				c = ' ';
				int color = TERM_NEUTRAL;
				//if(x==p->p1.x && y==p->p1.y)
				if(line==height-1)
				{
					if(b == p->p1.token)
					{
						color = TERM_RED;
						c = 'O';
					}
					//else if(x==p->p2.x && y==p->p2.y)
					else if(b == p->p2.token)
					{
						color = TERM_BLUE;
						//c = ry? 'R':'Y';
						c = 'O';
					}
				}
				term_fg(color);
				printf(" %c ", c);
				term_clear();

				//vert
				if(x < 8)
				{

					color = TERM_NEUTRAL;
					c = ':';
					if(b & p->vert)// || b>>9 & p->vert)
					{
						c = 186;
						color = TERM_PURPLE;
					}
					term_fg(color);
					printf("%c", c);
					term_clear();
				}

				b <<= 1;
			}

			b >>= 9;
		}

		if(!y)
			break;

		b >>= 9;

		//separation/horiz gates
		printf("\n%s  %d ", indent, y);
		for(int x=0; x<9; x++)
		{
			char color = TERM_NEUTRAL;
			c = '.';
			if(b & p->horiz)// || b>>1 & p->horiz)
			{
				c = 205;
				color = TERM_PURPLE;
			}
			term_fg(color);
			for(int i=0; i<4; i++)
				putchar(c);
			term_clear();

			b <<= 1;
		}
		b >>= 9;


	}

	//footer
	printf("\n%s       ", indent);
	for(int x=0; x<8; x++)
		printf("%c   ", 'A'+x);

	printf("\n\n\t\t\t\t\t");
	term_fg(TERM_RED);
	printf("red player: %d gates\t\t\t", p->p1.gate_ct);
	term_fg(TERM_BLUE);
	printf("blue player: %d gates\n\n", p->p2.gate_ct);
	term_clear();

	printf("\n\n");
	print_player_info(&p->p1);
	print_player_info(&p->p2);
	printf("horiz: ");
	printbig(p->horiz, "%x");
	printf("\nvert: ");
	printbig(p->vert, "%x");
	//printbig(0b11011, "%b");
}

int quor_human_to_iter(char *human)
{
	if(human[0] == '-' || human[0] == '|')
	{
		int iter = (human[1]-'a');
		iter += (human[2]-'0'-1)*9;
		if(human[0] == '-')
			iter += 4;
		else
			iter += HORIZ_PLACEMENTS;
		return iter;
	}
	else
	{
		switch(human[0])
		{
			case '^': return MOVE_UP;
			case 'v': return MOVE_DOWN;
			case '<': return MOVE_LEFT;
			case '>': return MOVE_RIGHT;
			default:	return -1;
		}
	}
	return -1;
}

char *quor_iter_to_human(int move)
{
	if(move < TOKEN_MOVES)
	{
		switch(move)
		{
			case MOVE_UP:	return "^";
			case MOVE_DOWN:	return "v";
			case MOVE_LEFT:	return "<";
			case MOVE_RIGHT:	return ">";
		}
	}
	else
	{
		int gate_index = move - TOKEN_MOVES;
		if(gate_index > 72)
			gate_index -= 72;
		static char gmove[4];
		gmove[0] = (move < HORIZ_PLACEMENTS)? '-':'|';
		gmove[1] = (gate_index % 9) + 'a';
		gmove[2] = (gate_index / 9) + '0' + 1;
		gmove[3] = '\0';
		return gmove;
	}

	return NULL;
}


solver_t QUOR_SOLVER =
{
	.name = "quoridor",

	.initial_pos = &QUOR_INIT_POS,
	.pos_size = sizeof(quor_pos_t),
	.possible_moves = 148,
	.transtbl_buckets_ct = (1<<28),
	.iddfs_increment = 2,
	.aspiration_default_width = 1,
	//.default_order = (uint8_t[]){3, 2, 4, 1, 5, 0, 6},
	.default_order = NULL,
	.flip_depth = 16,

	.gameover = quor_gameover,
	.whosemove = quor_whosemove,
	.is_legal = quor_is_legal,
	.make_move = quor_make_move,
	//.only_moves = quor_only_moves,
	.draw_full = quor_draw_full,

	.estimate = quor_estimate,


	.print_pos = NULL,
	.hash = quor_hash,
	.uses_zobrist = true,
	.keys_match = quor_keys_match,
	//.normalize_position = quor_normalize,
	//.flip = quor_flip,
	//.normalize_position = NULL,
	//.replace_transpose = quor_replace_transpose,

	.human_to_iter = quor_human_to_iter,
	.iter_to_human = quor_iter_to_human,
};
