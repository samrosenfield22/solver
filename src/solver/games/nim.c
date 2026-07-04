

#include "nim.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "../zobrist.h"
#include "../../utils/utils.h"

nim_pos_t NIM_INIT_POS =
{
	.piles = {3, 5, 7},
	.whosemove = true
};


//statics
int nim_human_to_iter(char *human);
void nim_calc_pile_and_take(void *pos, int index, int *pile, int *take);


/*bool nim_is_end(void *pos)
{
	nim_pos_t *np = pos;
	return (np->piles[0]==0 && np->piles[1]==0 && np->piles[2]==0);
}*/

endstate_t nim_gameover(void *pos)
{
	nim_pos_t *np = pos;

	if(np->piles[0]==0 && np->piles[1]==0 && np->piles[2]==0)
		return np->whosemove? END_P1_WON : END_P2_WON;
	else
		return 0;
}

bool nim_is_legal(void *pos, int index)
{
	nim_pos_t *np = pos;

	//printf("\tchecking legality for pos {%d,%d,%d}\n", np->piles[0], np->piles[1], np->piles[2]);


	int pile=0, take=0;
	nim_calc_pile_and_take(np, index, &pile, &take);

	return (take <= np->piles[pile]);
}

bool nim_whosemove(void *pos)
{
	nim_pos_t *np = pos;
	return np->whosemove;
}

void nim_make_move(void *pos, int index, uint64_t *hash)
{

	nim_pos_t *np = pos;
	//np->move_index = index;
	//printf("checking pos {%d,%d,%d}\n", np->piles[0], np->piles[1], np->piles[2]);

	int pile=0, take=0;
	nim_calc_pile_and_take(np, index, &pile, &take);

	np->piles[pile] -= take;

	np->whosemove = !np->whosemove;
	//printf("\t\t\t\t\tnew pos: {%d,%d,%d}\n", np->piles[0], np->piles[1], np->piles[2]);
}

/*int16_t nim_evaluate_leaf(void *pos)
{
	nim_pos_t *np = pos;

	if(np->piles[0]==0 && np->piles[1]==0 && np->piles[2]==0)
		return np->whosemove? 100 : -100;
	else
		return 0;
}*/


uint64_t nim_hash(void *key, size_t size)
{
	nim_pos_t *np = key;
	uint64_t h = 48*np->piles[0] + 8*np->piles[1] + np->piles[2];
	if(np->whosemove)
		h += 200;
	return h;
}

int nim_moves_remaining(void *pos)
{
	nim_pos_t *p = pos;
	int left = 0;
	for(int i=0; i<3; i++)
		left += p->piles[i];
	return left;
}

bool nim_keys_match(void *k1, void *k2)
{
	nim_pos_t *n1 = k1, *n2 = k2;
	return ((n1->piles[0] == n2->piles[0])
		&& (n1->piles[1] == n2->piles[1])
		&& (n1->piles[2] == n2->piles[2])
		&& (n1->whosemove == n2->whosemove));
}

/*void nim_normalize(void *k)
{
	nim_pos_t *n = k;
	uint8_t temp;
	//printf("was: %d,%d,%d\n", n->piles[0], n->piles[1], n->piles[2]);
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
	if(n->piles[2] < n->piles[1])
	{
		temp = n->piles[1];
		n->piles[1] = n->piles[2];
		n->piles[2] = temp;
	}
	//printf("is: %d,%d,%d\n", n->piles[0], n->piles[1], n->piles[2]);

}*/

void nim_draw_withmissing(void *pos, int row, int nmissing)
{
	nim_pos_t *p = pos;

	char indent[] = "\t\t\t\t\t\t\t";
	char space[] = "    ";
	int max[3] = {3, 5, 7};

	printf("\n\n\n\n\n\n");
	for(int i=0; i<3; i++)
	{
		printf("%s%s%c    ", TERM_WHITE, indent, 'A'+i);
		int gap = 7 - max[i];
		gap /= 2;
		for(int g=0; g<gap; g++)
			printf(space);
		for(int g=0; g<max[i]; g++)
		{
			char *color = (i==row && g >= p->piles[i]-nmissing)?
				TERM_RED : TERM_WHITE;
			printf("%s%c%s", color, (g < p->piles[i])? '*':' ', space);
		}

		printf("\n\n");
	}
}

void nim_draw_full(void *pos, int last_move)
{
	nim_draw_withmissing(pos, 0, 0);
}

void *nim_menu_define(void)
{
	menu_t *m = menu_grid(68, 19,	//x,y
		3, 1,	//rows,columns
		2, 4,	//r/c spacing
		"");

	//for(int i=0; )
	menu_set(m, 0);
	return m;
}

void redraw_nim_pos(void *pos, int current, int rocks_selected)
{
	window_unfocus();
	term_move_cursor(0, 12);
	nim_draw_withmissing(pos, current, rocks_selected);
}

int nim_menu_update(void *menu, void *pos, int key)
{
	nim_pos_t *p = pos;
	static int rocks_selected = 1;
	static int current = 0;
	static int last_piles[3] = {0, 0, 0};
	bool redraw = false;
	if(p->piles[0]!=last_piles[0]
		|| p->piles[1]!=last_piles[1]
		|| p->piles[2]!=last_piles[2])
	{
		redraw = true;
	}
	int piles = p->piles[current];
	if(piles == 0)
	{
		for(int i=0; i<3; i++)
			if(p->piles[i])
			{
				current = i;
				menu_set(menu, current);
				redraw = true;
				break;
			}
	}

	last_piles[0] = p->piles[0];
	last_piles[1] = p->piles[1];
	last_piles[2] = p->piles[2];


	if(redraw)
	{
		rocks_selected = 1;
		redraw_nim_pos(pos, current, rocks_selected);
	}

	bool pressed = true;
	char buf[3];
	buf[2] = '\0';
	switch(key)
	{
		case ARROW_UP:
		case ARROW_DOWN:
			menu_input_control(menu, key);
			current = menu_get(menu);
			if(p->piles[current] == 0 && current==1)
				menu_input_control(menu, key);
			//rocks_selected = 1;
			break;
		case ARROW_LEFT:	rocks_selected+=1;	break;
		case ARROW_RIGHT:	rocks_selected-=1;	break;
		case '\n':
		case '\r':
			buf[0] = 'A' + menu_get(menu);
			buf[1] = '0' + rocks_selected;
			//printf("(%s)\n", buf);
			//exit(0);
			return nim_human_to_iter(buf);
			break;
		default:
			pressed = false;
	}

	if(pressed)
	{

		current = menu_get(menu);
		piles = p->piles[current];

		//rocks_selected += d;
		if(rocks_selected <= 0)
			rocks_selected = 1;
		if(rocks_selected >= piles)
			rocks_selected = piles;

		redraw_nim_pos(pos, current, rocks_selected);
	}


	return -1;
}

int nim_human_to_iter(char *human)
{
	if(human[0] >= 'a')
		human[0] -= ('a'-'A');
	int pile = human[0]-'A';
	int max[] = {3,5,7};
	//int max_pile = max[pile];
	int take = max[pile] - (human[1]-'0');
	int move = 0;
	if(pile > 0)	move += 3;
	if(pile > 1)	move += 5;
	move += take;
	return move;
}

/*int nim_only_move(void *pos)
{
	nim_pos_t *p = pos;

	//p->piles[i]
	int last_pile = -1;
	int filled = 0;
	for(int i=0; i<3; i++)
	{
		if(p->piles[i])
		{
			filled++;
			if(last_pile == -1)
				last_pile = i;
			else
			{
				break;
			}
		}
	}

	if(filled == 1)
	{
		int cnt = p->piles[last_pile];
		printf("found last pile %d w %d\n", last_pile, cnt);
		if(cnt > 1)
		{
			char hmove[3];
			hmove[0] = 'A' + last_pile;
			hmove[1] = '0' + cnt-1;
			hmove[2] = '\0';
			printf("only move %s\n", hmove);
			return nim_human_to_iter(hmove);
		}
	}

	//check if opp has a win we have to block on next move


	return -1;
}*/

solver_t NIM_SOLVER =
{
	.name = "nim",

	.initial_pos = &NIM_INIT_POS,
	.pos_size = sizeof(nim_pos_t),
	.possible_moves = 15,
	.default_order = (uint8_t[]){9, 12, 11, 3, 6, 8, 10, 14, 0, 1, 3, 4, 7, 9, 13},
	.aspiration_default_width = 1,

	.gameover = nim_gameover,
	.estimate = NULL,
	.whosemove = nim_whosemove,
	.is_legal = nim_is_legal,
	.make_move = nim_make_move,
	.only_moves = NULL,

	.hash = nim_hash,
	.moves_remaining = nim_moves_remaining,
	.uses_zobrist = false,

	.menu_define = nim_menu_define,
	.menu_update = nim_menu_update,
	.draw_full = nim_draw_full,
	.human_to_iter = nim_human_to_iter,
	.iter_to_human = NULL,
};


void nim_calc_pile_and_take(void *pos, int index, int *pile, int *take)
{
	//printf("\t\tplaying move %d\n", index);
	int pile_ct[] = {3, 5, 7};
	int total = 0;
	for(int i=0; i<3; i++)
	{
		total += pile_ct[i];
		if(index < total)
		{
			*pile = i;
			*take = total - index;
			//printf("\t\ttaking %d from pile %d\n", *take, *pile);
			break;
		}


	}
}
