

#include "nim.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>




nim_pos_t NIM_INIT_POS =
{
	.piles = {3, 5, 7},
	.whosemove = true
};


//statics
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


	int pile, take;
	nim_calc_pile_and_take(np, index, &pile, &take);

	return (take <= np->piles[pile]);
}

bool nim_whosemove(void *pos)
{
	nim_pos_t *np = pos;
	return np->whosemove;
}

void nim_make_move(void *pos, int index, uint32_t *hash)
{

	nim_pos_t *np = pos;
	//np->move_index = index;
	//printf("checking pos {%d,%d,%d}\n", np->piles[0], np->piles[1], np->piles[2]);

	int pile, take;
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

int nim_print_pos(void *pos)
{
	nim_pos_t *np = pos;
	return printf("%d,%d,%d", np->piles[0], np->piles[1], np->piles[2]);
}

uint32_t nim_hash(void *key, size_t size)
{
	nim_pos_t *np = key;
	uint32_t h = 48*np->piles[0] + 8*np->piles[1] + np->piles[2];
	if(np->whosemove)
		h += 200;
	return h;
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

void nim_draw_full(void *pos)
{
	nim_pos_t *p = pos;

	char indent[] = "\t\t\t\t\t\t\t";
	char space[] = "    ";
	int max[3] = {3, 5, 7};

	printf("\n\n\n\n\n\n");
	for(int i=0; i<3; i++)
	{
		printf("%s%c    ", indent, 'A'+i);
		int gap = 7 - max[i];
		gap /= 2;
		for(int g=0; g<gap; g++)
			printf(space);
		for(int g=0; g<max[i]; g++)
			printf("%c%s", (g < p->piles[i])? '*':' ', space);

		printf("\n\n");
	}
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
	.transtbl_buckets_ct = 5000,
	//.default_order = (uint8_t[]){7, 14, 1, 2, 6, 13, 0, 5, 12, 4, 11, 3, 10, 9, 8},
	.default_order = (uint8_t[]){9, 12, 11, 3, 6, 8, 10, 14, 0, 1, 3, 4, 7, 9, 13},

	.gameover = nim_gameover,
	.estimate = NULL,
	.whosemove = nim_whosemove,
	//.is_end = nim_is_end,
	.is_legal = nim_is_legal,
	.make_move = nim_make_move,
	.only_moves = NULL,
	//.get_move = nim_get_move,
	//.evaluate_leaf = nim_evaluate_leaf,

	.print_pos = nim_print_pos,
	.hash = nim_hash,
	.uses_zobrist = false,
	.keys_match = nim_keys_match,
	//.normalize_position = nim_normalize,
	//.normalize_position = NULL,

	.draw_full = nim_draw_full,
	.human_to_iter = nim_human_to_iter,
	//.iter_to_human = nim_iter_to_human,
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
