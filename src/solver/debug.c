

#include "debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "shared.h"
#include "transposition.h"
#include "ui/play_windows.h"
#include "../utils/misc/windowing.h"
#include "../utils/misc/winterm.h"

//
void launch_explorer(void *pos);
void print_caught(void *pos, char *msg);

void catch_pos(void *pos, char *msg)
{
	print_caught(pos, msg);
	launch_explorer(pos);
}

void print_caught(void *pos, char *msg)
{
	window_unfocus();
	term_move_cursor(0, 12);
	solver->draw_full(pos, -1);
	printf("%s%s", TERM_WHITE, TERM_BLACK_BG);
	term_move_cursor(0, 28);
	for(int i=0; i<16; i++)
		printf(" ");
	term_move_cursor(0, 28);
	if(msg)
		printf("\n%s", msg);
	printf("\n(caught pos, press any key)");



	//dump pos binary
	/*uint64_t *pp = (uint64_t *)pos;
	term_move_cursor(0, 35);
	for(int i=0; i<solver->pos_size/8; i++)
	{
		printf("%s\n", sprintbig(*pp, "%b"));
		pp++;
	}*/

	//print tt info
	term_move_cursor(0, 35);
	gdata_t *gd = malloc(gdata_size);
	memcpy(gd->pos, pos, solver->pos_size);
	gd->hash = solver->hash(gd->pos, 0);
	trans_value_t ttval;
	bool got = tt_get(&ttval, gd, 0);
	if(got)
	{
		printf("--- tt ---\n");
		printf("score:\t%.1f\n", ttval.score);
		printf("best move:\t%d\n", ttval.best_move);
		printf("bound:\t");
		switch(ttval.bound)
		{
			case BOUND_EXACT:	printf("exact\n");	break;
			case BOUND_LOWER:	printf("lower\n");	break;
			case BOUND_UPPER:	printf("upper\n");	break;
			default:			printf("invalid\n");
		}
	}
	else
		printf("--- no tt ---\n");
	free(gd);
}

void launch_explorer(void *pos)
{
	//stack
	const int stack_len = 16;
	void *pos_stack[stack_len];
	int pos_n = 0;

	//push pos
	pos_stack[pos_n] = malloc(solver->pos_size);
	memcpy(pos_stack[pos_n], pos, solver->pos_size);
	//pos_n++;

	bool done = false;
	while(!done)
	{
		//get user input
		int in = getchar();
		switch(in)
		{
			case '\n':
			case '\r':
			case 'x':
				done = true; break;

			case 'b':
				if(!pos_n)
					break;
				//pop, redraw
				free(pos_stack[pos_n]);
				pos_n--;
				print_caught(pos_stack[pos_n], NULL);
				break;

			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
				//push new pos
				pos_n++;
				pos_stack[pos_n] = malloc(solver->pos_size);
				memcpy(pos_stack[pos_n], pos_stack[pos_n-1], solver->pos_size);

				//make move
				in -= '0';
				assert(solver->is_legal(pos_stack[pos_n], in));
				solver->make_move(pos_stack[pos_n], in, NULL);

				//redraw
				print_caught(pos_stack[pos_n], NULL);

				break;

		}

		fflush(stdin);
	}

	//we're done
	for(int i=0; i<=pos_n; i++)
		free(pos_stack[i]);
	//getchar();
	window_focus(analysis_hdl);
}
