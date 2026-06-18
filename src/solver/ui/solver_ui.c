

#include "solver_ui.h"

#include "../../utils/utils.h"
#include "../transposition.h"
#include "play_windows.h"

#include <string.h>

//statics
void print_eval_bar(float score);
void print_variations(gdata_t *gd, int len);
void print_score(float score);

#define printf(fmt, ...)	window_printf(fmt, ##__VA_ARGS__)

void print_evaluation(gdata_t *gd, int iddfs_depth, result_t result)
{
	static float last_score = 111111;

	window_focus(eval_hdl);
	if(result.score == last_score)
		window_cursor_set(3);
	else
	{
		last_score = result.score;
		window_cursor_set(0);
		print_eval_bar(result.score);
	}

	printf("\t\t  (depth: %d) \n\n", iddfs_depth);

	//print variation(s)
	#ifdef USE_TRANSPOSITION_TABLE
	//int var_len = min(iddfs_depth, 2*VARIATION_LENGTH);
	//print_variations(gd, var_len);
	print_variations(gd, 2*VARIATION_LENGTH);
	#endif
}

void print_eval_bar(float score)
{

	//window_cursor_set(0);

	int len = 40;
	char *indent = "   ";

	printf(indent);
	for(int i=0; i<len/2-1; i++)
		printf(" ");
	print_score(score);
	printf("        \n");

	printf(indent);
	printf("[");
	int maxd = 20;
	float dist = (score+maxd)/(2*maxd);
	if(dist > maxd) dist = maxd;
	if(dist < -maxd) dist = -maxd;
	dist *= len;

	for(int i=0; i<len; i++)
	{
		if(i < (int)dist)
			printf("%c", 219);
		else
			printf(" ");
	}
	printf("]\n");

	printf(indent);
	for(int i=0; i<len/2; i++)
		printf(" ");
	printf("^\n");

}

void draw_tt_load(void)
{
	const int load_bar_len = 25;
	const int load_bar_h = 16;
	const int load_num_h = load_bar_h - 3;

	static bool first = true;
	if(first)
	{
		first = false;
		window_unfocus();
		term_move_cursor(0, load_num_h);
		printf("0%%");
		term_move_cursor(0, load_bar_h-1);
		printf("^");
		term_move_cursor(0, load_bar_h+load_bar_len);
		printf("v");
		window_focus(analysis_hdl);
	}

	static int last_load = 0;
	int load = tt_load();
	if(load == last_load)
		return;
	last_load = load;

	printf(TERM_WHITE);
	window_unfocus();
	term_move_cursor(0, load_bar_h);
	for(int i=0; i<load/4; i++)
		printf("%c\n", 219);

	term_move_cursor(0, load_num_h);
	printf("%d%% ", load);

	window_focus(analysis_hdl);
}

typedef struct
{
	trans_value_t tv;
	int move;
} tv_with_last_move_t;

int pv_var_move;

int vars_from_root_compare(const void *aa, const void *bb)
{
	tv_with_last_move_t *a = (tv_with_last_move_t *)aa;
	tv_with_last_move_t *b = (tv_with_last_move_t *)bb;

	//if(!a->tv)	return 1;
	//if(!b->tv)	return -1;
	if(a->move == -1)	return 1;
	if(b->move == -1)	return -1;

	return b->tv.score - a->tv.score;
}

void print_variations(gdata_t *gd, int len)
{
	gdata_t *var_walker = mem_malloc(gdata_size);

	int vars = DISPLAY_VAR_CT;

	//load and sort the moves from root
	tv_with_last_move_t from_root[solver->possible_moves];
	if(vars > 1)
	{
		for(int i=0; i<solver->possible_moves; i++)
		{
			//copy original gd
			memcpy(var_walker, gd, gdata_size);

			//get tt info of the nth move
			if(solver->is_legal(&gd->pos, i))
			{
				make_new_move(var_walker, var_walker, i);
				//from_root[i].tv = tt_get(var_walker, 0);
				if(tt_get(&from_root[i].tv, var_walker, 0))
					from_root[i].move = i;
				else
					from_root[i].move = -1;
			}
			else
				//from_root[i].tv = NULL;
				from_root[i].move = -1;
		}


		//sort them
		qsort(from_root, solver->possible_moves,
			sizeof(from_root[0]), vars_from_root_compare);
	}
	else
	{
		memcpy(var_walker, gd, gdata_size);
		/*from_root[0].tv = tt_get(var_walker, 0);
		if(!from_root[0].tv)
			return;*/
		bool got = tt_get(&from_root[0].tv, var_walker, 0);
		if(!got)
			return;
		from_root[0].move = from_root[0].tv.best_move;
	}
	//assert(from_root[0].move != -1);

	for(int v=0; v<vars; v++)
	{
		//if(!from_root[v].tv)
		if(from_root[v].move == -1)
		{
			for(int i=0; i<2*VARIATION_LENGTH; i++)
				printf("     ");
			printf("\n");
			continue;
		}

		int spaces = printf("  #%d %+.1f", v+1, from_root[v].tv.score);
		for(int s=spaces; s<13; s++)
			printf(" ");

		int vmove = from_root[v].move;	//nth best
		float vscore = from_root[v].tv.score;

		//copy original gd
		memcpy(var_walker, gd, gdata_size);

		int i;
		for(i=0; i<len; i++)
		{
			//set variation to its best_move
			//variation[i] = vmove;

			//print var move
			if(solver->iter_to_human)
				printf(solver->iter_to_human(vmove));
			else
				printf("%d", vmove);

			//make the move
			make_new_move(var_walker, var_walker, vmove);

			//get the next one
			//trans_value_t *tv = tt_get(var_walker, 0);
			//if(!tv)
			trans_value_t tv;
			bool g = tt_get(&tv, var_walker, 0);
			if(!g)
			{
				if(vscore==WIN_SCORE-1 || vscore==-WIN_SCORE+1)
					printf("#");
				break;
			}
			printf(", ");
			vmove = tv.best_move;
			vscore = tv.score;
		}

		for(; i<2*VARIATION_LENGTH; i++)
			printf("    ");
		printf("\n");
	}

	mem_free(var_walker);
}

float abs_f(float a)
{
	return (a>=0)? a : -a;
}

void print_score(float score)
{
	if(abs_f(score) < MATE_LIMIT)
		printf("%.1f  ", score);
	else
	{
		int dif = WIN_SCORE - abs_f(score);
		//int m = dif/2;
		printf("M%d  ", dif);
	}
}
