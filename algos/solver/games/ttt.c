

#include "ttt.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>


ttt_pos_t TTT_INIT_POS =
{
	.spaces = {	EMPTY,		EMPTY,		EMPTY,
				EMPTY,		EMPTY,		EMPTY,
				EMPTY,		EMPTY,		EMPTY},
	.whosemove = true
};

endstate_t ttt_gameover(void *pos)
{
	ttt_pos_t *p = pos;



	//horizontal
	for(int i=0; i<9; i+=3)
	{
		if(p->spaces[i] != EMPTY)
			if(p->spaces[i] == p->spaces[i+1]
				&& p->spaces[i+1] == p->spaces[i+2])
				return (p->spaces[i]==X_PIECE)?
					END_P1_WON : END_P2_WON;
	}

	//vertical
	for(int i=0; i<3; i++)
	{
		if(p->spaces[i] != EMPTY)
			if(p->spaces[i] == p->spaces[i+3]
				&& p->spaces[i+3] == p->spaces[i+6])
				return (p->spaces[i]==X_PIECE)?
					END_P1_WON : END_P2_WON;
	}

	//diagonals
	if(p->spaces[0] != EMPTY)
		if(p->spaces[0] == p->spaces[4]
			&& p->spaces[4] == p->spaces[8])
			return (p->spaces[0]==X_PIECE)?
				END_P1_WON : END_P2_WON;


	if(p->spaces[2] != EMPTY)
		if(p->spaces[2] == p->spaces[4]
			&& p->spaces[4] == p->spaces[6])
			return (p->spaces[2]==X_PIECE)?
				END_P1_WON : END_P2_WON;

	//draw
	bool all_full = true;
	for(int i=0; i<9; i++)
	{
		if(p->spaces[i] == EMPTY)
		{
			all_full = false;
			break;
		}

	}
	if(all_full)
		return END_DRAW;

	//no win condition
	return END_NOT_OVER;
}

bool ttt_is_legal(void *pos, int index)
{
	ttt_pos_t *p = pos;
	return (p->spaces[index] == EMPTY);
}

bool ttt_whosemove(void *pos)
{
	ttt_pos_t *p = pos;
	return p->whosemove;
}

void ttt_make_move(void *pos, int index, uint32_t *hash)
{
	ttt_pos_t *p = pos;
	p->spaces[index] = (p->whosemove)? X_PIECE : O_PIECE;
	p->whosemove = !p->whosemove;
}

int ttt_print_pos(void *pos)
{
	ttt_pos_t *p = pos;
	for(int i=0; i<9; i++)
	{
		char c;
		switch(p->spaces[i])
		{
			case EMPTY: c = ' ';	break;
			case X_PIECE: c = 'X';	break;
			case O_PIECE: c = 'O';	break;
		}
		putchar(c);

		if((i%3)==2)
			putchar('|');
	}

	return 11;
}

uint32_t ttt_hash(void *key, size_t size)
{
	ttt_pos_t *p = key;
	uint32_t h = 0, mult = 1;
	for(int i=0; i<9; i++)
	{
		h += mult * p->spaces[i];
		mult *= 3;
	}
	if(p->whosemove)
		h += mult;
	return h;
}

bool ttt_keys_match(void *k1, void *k2)
{
	ttt_pos_t *n1 = k1, *n2 = k2;
	for(int i=0; i<9; i++)
	{
		if(n1->spaces[i] != n2->spaces[i])
			return false;
	}
	return n1->whosemove == n2->whosemove;
}

/*void ttt_normalize(void *k)
{
	ttt_pos_t *n = k;
	int temp;
	if(n->piles[1] < n->piles[0])
	{
		temp = n->piles[0];
		n->piles[0] = n->piles[1];
		n->piles[1] = temp;
	}
	if(n->piles[2] < n->piles[0])
	{
		temp = n->piles[0];
		n->piles[0] = n->piles[2];
		n->piles[2] = temp;
	}

}*/

void ttt_draw_full(void *pos)
{
	ttt_pos_t *p = pos;

	char indent[] = "\t\t\t\t\t\t\t\t";
	printf("%s   A   B   C\n\n", indent);
	for(int r=0; r<3; r++)
	{
		printf("%s%d  ", indent, r);
		for(int c=0; c<3; c++)
		{
			int piece = p->spaces[3*r+c];

			switch(piece)
			{
				case X_PIECE: putchar('X'); break;
				case O_PIECE: putchar('O'); break;
				case EMPTY: putchar(' '); break;
			}
			if(c != 2)
				printf(" | ");
		}
		if(r != 2)
			printf("\n%s   ---------\n", indent);
	}
}

int ttt_human_to_iter(char *human)
{
	if(human[0] >= 'a')
		human[0] -= ('a'-'A');
	int c = human[0]-'A';
	int r = human[1]-'0';
	int move = 3*r+c;
	return move;
}

char *ttt_iter_to_human(int move)
{
	return NULL;
}

solver_t TTT_SOLVER =
{
	.name = "tic-tac-toe",

	.initial_pos = &TTT_INIT_POS,
	.pos_size = sizeof(ttt_pos_t),
	.possible_moves = 9,
	.transtbl_buckets_ct = 10000,
	.default_order = (uint8_t[]){4, 0, 2, 6, 8, 1, 3, 5, 7},

	.gameover = ttt_gameover,
	.estimate = NULL,
	.whosemove = ttt_whosemove,
	.is_legal = ttt_is_legal,
	.make_move = ttt_make_move,

	.print_pos = ttt_print_pos,
	.hash = ttt_hash,
	.uses_zobrist = false,
	.keys_match = ttt_keys_match,
	//.normalize_position = ttt_normalize,
	.normalize_position = NULL,

	.draw_full = ttt_draw_full,
	.human_to_iter = ttt_human_to_iter,
	.iter_to_human = ttt_iter_to_human,
};
