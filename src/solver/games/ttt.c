

#include "ttt.h"
#include "../../utils/utils.h"
#include "../zobrist.h"

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

void ttt_make_move(void *pos, int index, uint64_t *hash)
{
	ttt_pos_t *p = pos;
	p->spaces[index] = (p->whosemove)? X_PIECE : O_PIECE;
	p->whosemove = !p->whosemove;
}

/*int ttt_print_pos(void *pos)
{
	ttt_pos_t *p = pos;
	for(int i=0; i<9; i++)
	{
		char c = ' ';
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
}*/

uint64_t ttt_hash(void *key, size_t size)
{
	ttt_pos_t *p = key;
	uint64_t h = 0, mult = 1;
	for(int i=0; i<9; i++)
	{
		h += mult * p->spaces[i];
		mult *= 3;
	}
	if(p->whosemove)
		h += mult;
	return h;
}

int ttt_moves_remaining(void *pos)
{
	ttt_pos_t *p = pos;
	int played = 0;
	for(int i=0; i<9; i++)
		if(p->spaces[i] != EMPTY)
			played++;
	return (9-played)/2;
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

void ttt_draw_full(void *pos, int last_move)
{
	ttt_pos_t *p = pos;

	char indent[] = "\t\t\t\t\t\t\t\t";
	printf("\n\n\n\n\n");
	printf("%s   A   B   C\n\n", indent);
	for(int r=0; r<3; r++)
	{
		printf("%s%d  ", indent, r);
		for(int c=0; c<3; c++)
		{
			printf((3*r+c == last_move)?
				TERM_BLUE_BG : TERM_BLACK_BG);

			int piece = p->spaces[3*r+c];

			switch(piece)
			{
				case X_PIECE: putchar('X'); break;
				case O_PIECE: putchar('O'); break;
				case EMPTY: putchar(' '); break;
			}
			printf(TERM_BLACK_BG);

			if(c != 2)
				printf(" | ");
		}
		if(r != 2)
			printf("\n%s   ---------\n", indent);
	}
}

void *ttt_menu_define(void)
{
	menu_t *m = menu_grid(68, 19,	//x,y
		3, 3,	//rows,columns
		2, 4,	//r/c spacing
		" <");

	//for(int i=0; )
	menu_set(m, 4);
	return m;
}

int ttt_menu_update(void *menu, void *pos, int key)
{
	if(key == '\n' || key == '\r')
		return menu_get(menu);

	ttt_pos_t *p = pos;
	int current = menu_get(menu);

	int d;
	switch(key)
	{
		case ARROW_UP:		d=-3;	break;
		case ARROW_DOWN:	d=3;	break;
		case ARROW_LEFT:	d=-1;	break;
		case ARROW_RIGHT:	d=1;	break;
		default:
			if(p->spaces[current]==EMPTY)
				return -1;
			for(int i=0; i<9; i++)
				if(p->spaces[i]==EMPTY)
				{
					menu_set(menu, i);
					return -1;
				}
			return -1;
	}
	int i;
	for(i=current+d; ; i+=d)
	{
		if(p->spaces[i]==EMPTY)
			break;
		if(i<0 || i>=9)
		{
			i = -1;
			break;
		}
	}

	if(i != -1)
		menu_set(menu, i);

	return -1;
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
	int col = move % 3;
	int row = move / 3;
	static char buf[3];
	snprintf(buf, 3, "%c%d", col + 'A', row);
	return buf;
}

solver_t TTT_SOLVER =
{
	.name = "tic-tac-toe",

	.initial_pos = &TTT_INIT_POS,
	.pos_size = sizeof(ttt_pos_t),
	.possible_moves = 9,
	.default_order = (uint8_t[]){1, 0, 1, 0, 2, 1, 0, 1, 0},
	.aspiration_default_width = 1,

	.gameover = ttt_gameover,
	.estimate = NULL,
	.whosemove = ttt_whosemove,
	.is_legal = ttt_is_legal,
	.make_move = ttt_make_move,

	.hash = ttt_hash,
	.moves_remaining = ttt_moves_remaining,
	.uses_zobrist = false,

	.draw_full = ttt_draw_full,
	.menu_define = ttt_menu_define,
	.menu_update = ttt_menu_update,
	//.human_to_iter = ttt_human_to_iter,
	.iter_to_human = ttt_iter_to_human,
};
