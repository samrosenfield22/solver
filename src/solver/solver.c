

#include "solver.h"

#include "../utils/utils.h"

#include "play_windows.h"
#include "clock.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <omp.h>

enum
{
	MAX_LAYER = true,
	MIN_LAYER = false
};

#define printf(fmt, ...)	window_printf(fmt, ##__VA_ARGS__)


typedef struct
{
	float score;
	bool full;
	int best_move;
} result_t;

typedef struct
{
	uint32_t hash;
	float score;
	int move_index;
	//uint8_t move_ct;
	uint8_t pos[];
} gdata_t;

//statics
void tt_create(void);
int tt_add(gdata_t *gd, result_t *result, int depth,
	int bound, int best_move);
bool set_aspiration_window(float *asp_window,
	float *asp_window_size, float *last_score, float score);
result_t eval(gdata_t *gd, int depth,
	float alpha, float beta, bool is_pv);
//int build_order(sorter_t *order, tree_t *gt, tnode_t *n, int depth);
int build_movelist(sorter_t *order, void *pos);
int sort_movelist(sorter_t *order, int len,
	gdata_t *gd, int depth);
bool move_is_forcing(void *pos, int move);

result_t analyze_all_children(gdata_t *gd, trans_value_t *ttval,
	sorter_t *order, int len, int depth, float alpha, float beta,
	bool is_pv);

bool tt_get(trans_value_t *value, gdata_t *gd, int depth);

bool alphabeta_cutoff(float cscore, float prune,
	float *best_so_far, int depth);
float max(float x, float y);
float min(float x, float y);
bool time_up(void);
void print_score(float score);


bool is_better(float s0, float s1, int depth);
bool is_worse(float s0, float s1, int depth);
float worst_score(int depth);
bool is_win_score(float s, int depth);
bool max_or_min(int depth);

//tnode_t *node_make_new_move(tree_t *gt, tnode_t *n, int move);
bool make_new_move(gdata_t *child, gdata_t *gd, int move);
void *node_get_pos(tnode_t *n);
uint32_t *node_get_hash(tnode_t *n);
uint32_t *gdata_get_hash(gdata_t *gd);

void print_eval_bar(float score);
void print_variations(gdata_t *gd, int len);

solver_t *solver;
hashmap_t *trans_tbl = NULL;
bool who_goes_first = true;
uint32_t position_ct = 0;
int iddfs_depth;
int time_lim;

size_t gdata_size, gdata_hash_size;

int *HISTORY_VALS;
uint8_t HISTORY_BEST[2] = {-1, -1};

#define MAX_PLY	(42)
#define NUM_KILLERS	(2)
uint8_t killers[MAX_PLY][NUM_KILLERS] = {0};



//uint32_t passed=0, checked=0;

void print_evaluation(gdata_t *gd, result_t result)
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
	int load = hashmap_load(trans_tbl);
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

bool asp_window_rerun = false;


void solver_check(solver_t *s)
{
	if(!(s->initial_pos)) {printf("error: solver missing initial_pos\n"); exit(0);}
	if(!(s->pos_size)) {printf("error: solver missing pos_size\n"); exit(0);}
	if(!(s->possible_moves)) {printf("error: solver missing possible_moves\n"); exit(0);}
	if(!(s->gameover)) {printf("error: solver missing gameover\n"); exit(0);}
	if(!(s->is_legal)) {printf("error: solver missing is_legal\n"); exit(0);}
	if(!(s->make_move)) {printf("error: solver missing make_move\n"); exit(0);}
	if(!(s->hash)) {printf("error: solver missing hash\n"); exit(0);}
	if(!(s->draw_full)) {printf("error: solver missing draw_full\n"); exit(0);}

	#ifdef ASPIRATION_WINDOW
	if(!(s->aspiration_default_width)) {printf("error: solver missing aspiration_default_width\n"); exit(0);}
	#endif

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

	//memcpy(var_walker, gd, gdata_size);
	/*trans_value_t *tv = tt_get(var_walker, 0);
	if(!tv)
		return;
	pv_var_move = tv->best_move;*/

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

int random_move(void *pos)
{
	int ct = 0;
	int allowed[solver->possible_moves];

	for(int i=0; i<solver->possible_moves; i++)
		if(solver->is_legal(pos, i))
			allowed[ct++] = i;
	assert(ct);

	uint8_t n = rand();
	n %= ct;
	return allowed[n];
}

void solver_init(solver_t *game_solver)
{
	solver_check(game_solver);

	solver = game_solver;
	if(!solver->hash_size)
		solver->hash_size = solver->pos_size;
	gdata_size = sizeof(gdata_t) + solver->pos_size;

	#ifdef USE_TRANSPOSITION_TABLE
	if(!trans_tbl)
		tt_create();
	#endif	//USE_TRANSPOSITION_TABLE
}

void solver_clear(void)
{
	hashmap_destroy(trans_tbl);
	trans_tbl = NULL;
}

float solve(solver_t *game_solver, void *pos, int init_depth,
	int time_lim_ms, bool verbose)
{
	//tic();

	time_lim = time_lim_ms;
	position_ct = 0;

	/*#ifdef USE_TRANSPOSITION_TABLE
	if(!trans_tbl)
		tt_create();
	#endif	//USE_TRANSPOSITION_TABLE*/

	if(!pos)
		pos = solver->initial_pos;
	gdata_t *gd = mem_malloc(gdata_size);
	gd->hash = solver->hash(pos, solver->pos_size);
	memcpy(&(gd->pos), pos, solver->pos_size);

	who_goes_first = solver->whosemove(pos);

	if(verbose)
	{
		if(solver->print_pos)
		{
			printf("solving game position: ");
			solver->print_pos(gd->pos);
			printf("\n");
		}
		else
			printf("solving ");
		#ifdef FORCE_SEARCH_DEPTH
		printf("(force search depth %d)", FORCE_SEARCH_DEPTH);
		#else
		printf("(time limit %d ms)", time_lim_ms);
		#endif
		printf("\n");

		//printf("pre setup time = %d ms\n", toc_ms());
	}
	tic();

	#ifdef USE_HISTORY_HEURISTIC
	int history_scores[solver->possible_placements];
	for(int i=0; i<solver->possible_placements; i++)
		history_scores[i] = 0;
	HISTORY_VALS = history_scores;
	#endif

	result_t result, last_result;
	float asp_window[] = {-WIN_SCORE, WIN_SCORE};
	float last_iddfs_score = 0;

	int cores = MULTICORE_CT;
	solver_t alt_solvers[cores-1];
	for(int i=0; i<cores-1; i++)
	{
		memcpy(&alt_solvers[i], game_solver, sizeof(solver_t));
		alt_solvers[i].default_order = mem_malloc(solver->possible_moves);
		for(int m=0; m<solver->possible_moves; m++)
			alt_solvers[i].default_order[m] = rand() / 2000;
	}

	//iddfs
	int iddfs, iddfs_complete=0;
	int incr = solver->iddfs_increment;
	if(!incr)
		incr = 2;
	for(iddfs=0;; iddfs+=incr)
	{
		#ifdef FORCE_SEARCH_DEPTH
		if(iddfs > FORCE_SEARCH_DEPTH)
			iddfs = FORCE_SEARCH_DEPTH;
		#endif
		iddfs_depth = iddfs;

		uint32_t last = toc_ms();


		float asp_window_size[] = {solver->aspiration_default_width,
								solver->aspiration_default_width};
		(void)asp_window_size;
		(void)last_iddfs_score;
		while(1)
		{

			printf("\niddfs=%d in window [%.1f,%.1f] ",
				iddfs, asp_window[0], asp_window[1]);

			if(MULTICORE_CT > 1)
			{
				//#pragma omp parallel for private(solver)
				#pragma omp parallel for
				for(int mp=0; mp<MULTICORE_CT; mp++)
				{
					//int tid = omp_get_thread_num();
					//printf("solving from thread %d\n", tid);
					//solver = (tid==0)? game_solver : &alt_solvers[tid-1];
					result = eval(gd, 0,
						asp_window[0], asp_window[1],
						true);
				}
				//exit(0);
			}
			else
			{
				result = eval(gd, 0,
					asp_window[0], asp_window[1],
					true);
			}


			if(time_up())
				break;

			bool in_window = true;
			#ifdef ASPIRATION_WINDOW
			in_window = set_aspiration_window(asp_window,
				asp_window_size, &last_iddfs_score,
				result.score);
			#endif
			if(in_window)
				break;
		}

		if(time_up())
		{
			result = last_result;
			printf("(time up)\n");
			break;
		}


		last_result = result;
		iddfs_complete = iddfs;

		//print info
		if(verbose)
		{
			print_evaluation(gd, result);

			uint32_t now = toc_ms();
			window_focus(analysis_hdl);
			printf("(%u ms)", now-last);
			last = now;
		}
		//printf("      ");
		window_focus(analysis_hdl);



		//conditions for ending the search
		if(result.full)
			break;

		#ifdef FORCE_SEARCH_DEPTH
			if(iddfs >= FORCE_SEARCH_DEPTH)
				break;
		#endif

		#ifdef USE_HISTORY_HEURISTIC
		for(int i=0; i<solver->possible_placements; i++)
			HISTORY_VALS[i] /= 2;
		#endif
	}

	//count time
	uint32_t us = toc();
	uint32_t ms = us/1000;
	uint32_t sec_total = ms/1000;
	uint32_t min = sec_total/60;
	uint32_t sec = sec_total - (60*min);



	int best_move = result.best_move;
	//assert(best_move != -1);
	if(best_move == -1)
		best_move = random_move(gd->pos);

	if(verbose)
	{
		printf("\n\n--- search ran to depth = %d%s ---\n",
			iddfs_complete, result.full? " (full solve)":"");
	}

	printf("best move: \t");
	if(solver->iter_to_human)
		printf("%s\n", solver->iter_to_human(best_move));
	else
		printf("%d\n", best_move);
	printf("evaluation:\t%+.1f\n", result.score);

	if(verbose)
	{
		if(ms < 1500)
			printf("\n\nposition solved in %d ms\n", ms);
		else
			printf("\n\nposition solved in %d m, %d sec\n", min, sec);
		printf("time per position: ");
		if(position_ct)
			printf("%.2f us\n", ((float)us)/position_ct);
		else
			printf("N/A\n");
		printf("evaluated %s unique positions\n", sprintbig(position_ct, "%d"));
		//printf("greatest number of nodes stored in tree: %u\n", max_node_ct);
		#ifdef USE_TRANSPOSITION_TABLE
		printf("hashmap load factor = %d%%\n", hashmap_load(trans_tbl));
		printf("number of collisions: %s\n", sprintbig(hashmap_collisions(trans_tbl), "%d"));
		#endif
	}


	//clean up
	mem_free(gd);
	//hashmap_destroy(trans_tbl);
	//trans_tbl = NULL;
	zobrist_free();


	//term_move_cursor(0, 15);

	#ifdef USE_HISTORY_HEURISTIC
	window_unfocus();
	term_move_cursor(0, 36);
	for(int c=5; c>=0; c--)
	{
		for(int r=0; r<7; r++)
		{
			int place = 6*r + c;
			int space = printf("%d:%d   ",
				place, HISTORY_VALS[place]);
			for(int i=0; i<10-space; i++)
				putchar(' ');
		}
		printf("\n");
	}
	printf("\n\n");
	for(int c=5; c>=0; c--)
	{
		for(int r=0; r<7; r++)
		{
			int place = 6*r + c + 42;
			int space = printf("%d:%d   ",
				place, HISTORY_VALS[place]);
			for(int i=0; i<10-space; i++)
				putchar(' ');
		}
		printf("\n");
	}
	#endif	//USE_HISTORY_HEURISTIC

	return best_move;
}

//returns true if we're in the window
bool set_aspiration_window(float *asp_window,
	float *asp_window_size, float *last_score, float score)
{
	bool in_window = (asp_window[0] < score
		&& score < asp_window[1]);

	//sus
	if(score==WIN_SCORE || score==-WIN_SCORE)
		in_window = true;

	if(in_window)
	{
		//set window params for next iddfs
		*last_score = score;
		asp_window_size[0] = solver->aspiration_default_width;
		asp_window_size[1] = solver->aspiration_default_width;
	}
	else
	{
		//test
		//asp_window[0] = -WIN_SCORE;
		//asp_window[1] = WIN_SCORE;
		//return false;

		printf("\n--- score %.1f out of window ---", score);
		/*if(score <= asp_window[0])
			asp_window_size[0] *= 2;
		else if(score >= asp_window[1])
			asp_window_size[1] *= 2;*/
		if(score <= asp_window[0])
			asp_window[0] = -WIN_SCORE;
		else if(score >= asp_window[1])
			asp_window[1] = WIN_SCORE;

		asp_window_rerun = true;
		return false;

		//printf("\t--- extending aspiration window size to %.1f,%.1f ---\n",
		//	asp_window_size[0], asp_window_size[1]);
	}

	asp_window[0] = *last_score - asp_window_size[0];
	asp_window[1] = *last_score + asp_window_size[1];

	asp_window_rerun = !in_window;
	return in_window;
}

//recursive
result_t eval(gdata_t *gd, int depth,
	float alpha, float beta, bool is_pv)
{
	//printf("eval at d=%d (%d)\n", depth, omp_get_thread_num());
	if(time_up())
		return (result_t){.score=0, .full=false, .best_move=-1};


	//if(is_pv)
	//	printf("\tpv node at d=%d w [%.1f,%.1f]\n",
	//	depth, alpha, beta);
	assert(gd);
	//assert(n->child_ct == 0);
	if(depth > 1000)
	{
		printf("error! depth is crazy big lol\n");
		exit(0);
	}
	//void *pos = node_get_pos(n);
	void *pos = &(gd->pos);

	//trans_value_t *ttval = NULL;
	trans_value_t ttval;
	bool got = false;
	#ifdef USE_TRANSPOSITION_TABLE
	//check if position was already analyzed
	//ttval = tt_get(gd, depth);
	//if(ttval)
	got = tt_get(&ttval, gd, depth);
	if(got)
	{
		gd->score = ttval.score;
		//if(ttval->full)
		if(ttval.full && ttval.bound==BOUND_EXACT)
		{
			assert(gd->score > MATE_LIMIT
				|| gd->score < -MATE_LIMIT
				|| gd->score == 0);
			return (result_t){.score=gd->score, .full=true, .best_move=ttval.best_move};
		}
		//if(ttval->iddfs >= iddfs_depth && !asp_window_rerun)
		//if(ttval->iddfs >= iddfs_depth && ttval->bound==BOUND_EXACT)
		//assert(!ttval->full);
		if(ttval.search_depth >= (iddfs_depth - depth)
			&& ttval.bound==BOUND_EXACT)
		//if(val->iddfs >= iddfs_depth
		//	&& !(asp_window_rerun && !val->exact))
			return (result_t){.score=gd->score, .full=false, .best_move=ttval.best_move};
	}
	#endif

	//update metrics
	if(!(position_ct & 0xFFFFF))	//every few 100k
	{
		//printf("%d\n", position_ct);
		draw_tt_load();
	}
	position_ct++;
	//if(gt->node_ct > max_node_ct)
	//	max_node_ct = gt->node_ct;



	//evaluate end states/leaf nodes
	int endstate = solver->gameover(pos);
	if(endstate != END_NOT_OVER)
	{
		switch(endstate)
		{
			case END_P1_WON:	gd->score = WIN_SCORE;	break;
			case END_P2_WON:	gd->score = -WIN_SCORE;	break;
			case END_DRAW:		gd->score = 0;			break;
			default:	printf("invalid endstate!\n"); exit(0);
		}
		return (result_t){.score=gd->score, .full=true, .best_move=-1};
	}
	else if(depth >= iddfs_depth)
	{
		if(solver->estimate)
			gd->score = solver->estimate(pos);
		else
			gd->score = 0;
		assert(-MATE_LIMIT < gd->score && gd->score < MATE_LIMIT);
		return (result_t){.score=gd->score, .full=false, .best_move=-1};
	}

	//make movelist
	//if there's only one move (that wins or loses),
	//just play that
	sorter_t movelist[solver->possible_moves];
	int len = 0;

	if(solver->only_moves)
		len = solver->only_moves(movelist, pos);

	if(!len)
	{
		if(solver->make_movelist)
			len = solver->make_movelist(movelist, pos);
		else
			len = build_movelist(movelist, pos);

		len = sort_movelist(movelist, len, gd, depth);
	}

	//main analysis -- recursive tree search
	trans_value_t *tv = got? &ttval : NULL;
	result_t result = analyze_all_children(gd, tv, movelist, len,
		depth, alpha, beta, is_pv);


	bool win = is_win_score(result.score, depth);
	if(win)
	{
		result.score -= (max_or_min(depth)==MAX_LAYER)? 1: -1;
	}


	#ifdef USE_TRANSPOSITION_TABLE
	int bound = BOUND_EXACT;
	if(!win)
	{
		if(alpha < result.score
			&& result.score < beta)
			bound = BOUND_EXACT;
		else if(result.score <= alpha)
		{
			bound = BOUND_UPPER;
			//result.full = false;
		}
		else
		{
			bound = BOUND_LOWER;
			//result.full = false;
		}
	}
	tt_add(gd, &result, depth, bound, result.best_move);
	#endif


	//gd->score = result.score;	//might not even need

	assert(result.best_move != -1);
	return result;
}

/*void history_bonus(void *pos, int move, int depth)
{
	int sdq = (iddfs_depth - depth) * (iddfs_depth - depth);
	int placement = solver->get_placement(pos, move);
	if(placement != -1)
		HISTORY_VALS[placement] += sdq;
	if(placement == 18)
		printf("incrementing 3cent by %d (d=%d)\n", sdq, depth);
}*/

void update_history(void *pos, int index,
	sorter_t *order, int len, int depth, bool was_cutoff)
{
	//int move = order[index].move;
	int sdq = (iddfs_depth - depth) * (iddfs_depth - depth);
	//sdq *= sdq;
	//sdq /= 256;
	if(sdq <= 0)
	{
		printf("iddfs_depth = %d, depth = %d, got sdq=%d\n",
			iddfs_depth, depth, sdq);
	}
	assert(sdq > 0);

	int placement = solver->get_placement(pos, order[index].move);
	assert(placement != -1);
	HISTORY_VALS[placement] += sdq;
	return;

	///////////////////////////

	for(int i=0; i<len; i++)
	{
		int placement = solver->get_placement(pos, order[i].move);
		if(placement == -1)
		{
			assert(i != index);
			continue;
		}

		if(i == index)
		{
			if(placement == 18)	printf("p18 += %d\n", sdq);
			HISTORY_VALS[placement] += sdq;
			if(was_cutoff)
				break;
		}
		else
		{
			//printf("decrementing pl %d from %d to ", placement,
			//	HISTORY_VALS[placement]);
			if(placement == 18)	printf("p18 -= %d\n", sdq);
			HISTORY_VALS[placement] -= sdq;
			//printf("%d\n", HISTORY_VALS[placement]);
		}

		/*if(HISTORY_VALS[move] >= HISTORY_THRESH)
		{

		}*/
	}
}

result_t analyze_all_children(gdata_t *gd, trans_value_t *ttval,
	sorter_t *order, int len, int depth, float alpha, float beta,
	bool is_pv)
{
	//assert(n->child_ct == 0);

	if(ttval && ttval->search_depth >= (iddfs_depth - depth))
	{
		if(ttval->bound == BOUND_LOWER)
			alpha = max(alpha, ttval->score);
		else if(ttval->bound == BOUND_UPPER)
			beta = min(beta, ttval->score);
	}
	if(alpha >= beta)	//bail immediately
	{
		//assert(ttval->full == false);
		return (result_t){.score=ttval->score, .full=ttval->full, .best_move=ttval->best_move};
	}

	float alpha_init = alpha, beta_init = beta;
	(void)alpha_init;
	(void)beta_init;

	result_t result;
	result_t best_result = {.score=worst_score(depth), .full=true, .best_move = order[0].move};


	bool is_max = (max_or_min(depth)==MAX_LAYER);

	bool multi_pv = (depth==0 && DISPLAY_VAR_CT>1);
	bool multi_full = true;

	/*int tid = omp_get_thread_num();
	if(tid && depth==2 && MULTICORE_CT > 1)
	{
		int swap_move = order[tid].move;
		order[tid].move = order[0].move;
		order[0].move = swap_move;
	}*/

	for(int i=0; i<len; i++)
	{
		//tree_get(gt, n);

		//get move to try
		int move = order[i].move;
		/*assert(move != -1);
		if(move == -1)
		{
			assert(len > 1);
			continue;
		}*/
		assert(0 <= move && move < solver->possible_moves);

		//make new position
		//tnode_t *child = node_make_new_move(gt, n, move);
		//gdata_t child;
		//gdata_t *child = sl_alloc(gdata_size);
		uint8_t child[gdata_size];
		bool made = make_new_move((gdata_t *)&child, gd, move);
		//if(!child)
		if(!made)
		{
			assert(len != 1);
			//assert(i);
			//sl_free(child);
			continue;
		}

		/*if(is_pv && i)
		{
			//printf("window tightened from [%.1f,%.1f] to", alpha, beta);
			if(is_max)
				beta = alpha+1;
			else
				alpha = beta-1;
			//printf("[%.1f,%.1f]\n", alpha, beta);
		}*/

		bool child_pv = is_pv && (i==0);
		result = eval((gdata_t *)&child, depth+1,
			alpha, beta, child_pv);
		//sl_free(child);

		if(multi_pv && !result.full)
			multi_full = false;

		#ifdef RETURN_FIRST_WIN_FOUND
		if(is_win_score(result.score, depth))
		{
			//best_full = true;
			best_result = result;
			best_result.best_move = move;


			//best = result.score;
			//best_index = n->child_ct-1;
			//best_move = move;

			#ifdef USE_HISTORY_HEURISTIC
			update_history(pos, i, order, len, depth, true);
			#endif

			goto analyze_end;

		}
		#endif	//RETURN_FIRST_WIN_FOUND


		if(is_better(result.score, best_result.score, depth))
		{
			best_result = result;
			best_result.best_move = move;
		}

		//if(!result.full)
		//	best_full = false;

		#ifdef USE_ALPHABETA_PRUNING

		#ifdef PRINCIPAL_VAR_SEARCH
		if(is_pv && i && !multi_pv)
		{
			assert(alpha+1 == beta);
			//if we failed high, rerun
			float high_limit = is_max? beta : alpha;
			//printf("\t\tchecking fail state w score=%.1f against lim=%.1f\n",
			//	result.score, high_limit);
			bool fail_high = is_better(result.score,
				high_limit, depth);
			if(fail_high)
			{
				//printf("------------ rerunning analysis at d=%d\n",
				//	depth);
				//exit(0);
				/*return analyze_all_children(gt, n, order,
					len, depth, alpha_init, beta_init,
					false);*/
				is_pv = false;
				alpha = alpha_init;
				beta = beta_init;
				i--;
				continue;
			}
		}
		#endif	//PRINCIPAL_VAR_SEARCH

		if(!multi_pv)
		{
			if(is_max)
				alpha = max(alpha, result.score);
			else	//min
				beta = min(beta, result.score);
		}

		if(alpha >= beta)
		{
			//update history
			#ifdef USE_HISTORY_HEURISTIC
			update_history(pos, i, order, len, depth, true);
			#endif

			//update killers
			/*assert(depth < MAX_PLY);
			if(solver->only_moves
				&& solver->only_moves(NULL, child)==0)	//non forcing
			{
				uint8_t *killers_ply = killers[depth];
				if(killers_ply[0] != move)
				{
					killers_ply[1] = killers_ply[0];
					killers_ply[0] = move;
				}
			}*/

			break;
		}
		/*else
		{
			int placement = solver->get_placement(node_get_pos(n), order[i].move);
			assert(placement != -1);
			HISTORY_VALS[placement] -= 3;
		}*/

		#ifdef PRINCIPAL_VAR_SEARCH
		if(is_pv && (i==0))
		{
			if(!multi_pv)
			{
				if(is_max)
					beta = alpha+1;
				else
					alpha = beta-1;
				//printf("[%.1f,%.1f]\n", alpha, beta);
			}
		}
		#endif	//PRINCIPAL_VAR_SEARCH
		#endif	//USE_ALPHABETA_PRUNING

	}

	/*if(alpha < beta)
	{
		#ifdef USE_HISTORY_HEURISTIC
		update_history(n, best_index, order, len, depth, false);
		#endif
	}*/


	goto analyze_end;
	analyze_end:
	if(!(0 <= best_result.best_move
		&& best_result.best_move < solver->possible_moves))
	{
		printf("\nlen=%d, move=%d\n", len, best_result.best_move);
		printf("score went from %.1f to %.1f\n", worst_score(depth), best_result.score);
		assert(0);
	}



	//no legal moves??
	//assert(n->child_ct);
	//assert(n->children[0]);
	//assert(n->children[0]->score == best);
	//assert(is_win_score(best, depth) == best_full);

	//n->score = best;	//fail soft

	//in multi pv, all nodes need to fully evaluate to depth
	if(multi_pv && !multi_full)
		best_result.full = false;

	//return (result_t){.score=best, .full=best_full, .best_move=best_move};
	return best_result;
}

int order_compare(const void *aa, const void *bb)
{
	const sorter_t *a = aa;
	const sorter_t *b = bb;

	return (a->score > b->score)? -1 : 1;
}

int build_movelist(sorter_t *order, void *pos)
{
	int ct = 0;

	for(int i=0; i<solver->possible_moves; i++)
	{
		//get move, going in default order
		//int move = solver->default_order?
		 //	solver->default_order[i] : i;
		if(!solver->is_legal(pos, i))
			continue;
		order[ct].move = i;
		order[ct].score = 0;
		ct++;
	}

	//must have found at least 1 legal move
	assert(ct);
	return ct;
}


int sort_movelist(sorter_t *order, int len, gdata_t *gd, int depth)
{
	void *pos = &(gd->pos);

	//trans_value_t *v = tt_get(gd, depth);
	//int best = v? v->best_move : -1;
	trans_value_t v;
	bool got = tt_get(&v, gd, depth);
	int best = got? v.best_move : -1;

	uint8_t *killers_ply = killers[depth];
	(void)killers_ply;


	#ifdef USE_HISTORY_HEURISTIC
	int history_best, history_second, history_thresh=-100;
	bool history_set = false;
	for(int i=0; i<solver->possible_moves; i++)
	{
		int placement = solver->get_placement(pos, i);
		if(placement == -1)
			continue;
		if(HISTORY_VALS[placement])
		{
			history_set = true;
		}
		if(HISTORY_VALS[placement] >= history_thresh)
		{
			history_best = i;
			history_thresh = HISTORY_VALS[placement];
		}
	}
	if(!history_set)
		history_best = -1;
	history_thresh = -100;
	for(int i=0; i<solver->possible_moves; i++)
	{
		if(i == history_best)
			continue;
		int placement = solver->get_placement(pos, i);
		if(HISTORY_VALS[placement] >= history_thresh)
		{
			history_second = i;
			history_thresh = HISTORY_VALS[placement];
		}
	}

	#endif	//USE_HISTORY_HEURISTIC


	//if(history_best != -1)
	//	printf("best history val is %d\n", history_best);


	//printf("killers are %d and %d\n", killers_ply[0], killers_ply[1]);

	int losers = 0;

	for(int i=0; i<len; i++)
	{
		//get move
		int move = order[i].move;
		assert(move < solver->possible_moves);
		assert(order[i].score == 0);
		//assert(solver->is_legal(pos, move));

		//heuristic bonuses
		if(move == best)	//hash move
			order[i].score += 10000;
		#ifdef USE_HISTORY_HEURISTIC
		else if(move == history_best)
			order[i].score += 700;
		else if(move == history_second)
			order[i].score += 600;
		#endif	//USE_HISTORY_HEURISTIC
		else if(move_is_forcing(pos, move))
			order[i].score += 800;



		//else if(move == killers_ply[0]
		//	|| move == killers_ply[1])
		//	order[ct].score = 400;

		//add default move ordering
		if(solver->default_order)
			order[i].score += solver->default_order[move];


		if(solver->move_loses && solver->move_loses(pos, move))
		{
			order[i].score = -100;
			losers++;
		}

	}

	//sort
	qsort(order, len, sizeof(*order), order_compare);

	/*if(losers)
	{
		printf("\nscores: ");
		for(int i=0; i<len; i++)
			printf("[%d: %.1f], ", order[i].move, order[i].score);
		printf("         \n%d losers\n", losers);
		window_unfocus();
		term_move_cursor(0, 12);
		solver->draw_full(pos, -1);
		getchar();
	}*/

	assert(len >= losers);
	if(losers)
		assert(order[len-1].score == -100);
	/*len -= losers;
	if(!len)
		len = 1;*/


	return len;
}

bool move_is_forcing(void *pos, int move)
{
	if((!solver->make_move_temp) || (!solver->only_moves))
		return false;

	//int only = solver->only_move(solver->make_move_temp(pos, move));

	uint8_t after[solver->pos_size];
	//memcpy(after, pos, solver->pos_size);
	//solver->make_move(after, move, NULL);
	solver->make_move_temp(&after, pos, move, NULL);
	int len = solver->only_moves(NULL, after);
	return (len > 0);
}


//1 (old val) gets replaced with 2 (new val)
bool tt_replace_by_depth(void *k_old, void *v_old,
	void *k_new, void *v_new)
{
	//return false;

	trans_value_t *val_old = v_old;
	trans_value_t *val_new = v_new;

	return (val_new->search_depth >= val_old->search_depth);
}

void tt_create(void)
{

	//gdata_hash_size = sizeof(gdata_t) + solver->hash_size;

	//printf("tt w ksize=%d, vsize=%d\n", solver->hash_size, sizeof(trans_value_t));
	//exit(0);

	//create the table
	trans_tbl = hashmap_create(solver->hash_size,
		sizeof(trans_value_t),
		solver->transtbl_buckets_ct);
	if(!trans_tbl)
	{
		printf("failed to allocate transposition table\n");
		exit(0);
	}
	if(MULTICORE_CT > 1)
		hashmap_enable_multithread(trans_tbl);
	if(solver->hash)
		hashmap_attach_hash(trans_tbl, solver->hash);
	if(solver->keys_match)
		hashmap_attach_keycompare(trans_tbl,
			solver->keys_match);
	//if(solver->normalize_position)
	//	hashmap_attach_normalize(trans_tbl, solver->normalize_position);
	//if(solver->replace_transpose)
	//	hashmap_attach_replace(trans_tbl, solver->replace_transpose);
	hashmap_attach_replace(trans_tbl, tt_replace_by_depth);

	//test the hash table functionality with the current solver
	/*tree_t *test = tree_create(solver->pos_size);
	tree_add(test, solver->initial_pos);
	for(int i=0; i<4; i++)
	{
		int move = rand() % solver->possible_moves;
		if(solver->is_legal(node_get_pos(test->p), move))
			solver->make_move(node_get_pos(test->p), move);
		printf("(made move %d)\n", move);
	}
	//solver->print_pos(test->p->data);
	//solver->print_pos(test->p->data);
	trans_value_t *val = tt_get(test->p);
	assert(!val);

	int add_result = tt_add(test->p, 67, 4, -1);
	assert(add_result == HM_ADDED_KV_NEW);
	val = tt_get(test->p);
	assert(val);
	assert(val->iddfs == iddfs);
	assert(val->score == 67);
	assert(val->depth == 4);

	tree_clear(test);
	//exit(0);
	hashmap_clear(trans_tbl);
	*/
	/*
	create a position
	assert that it's not found in the table
	add it to the table
	assert that it is found, and that all the values are correct
	normalize it, same
	*/
}

int tt_add(gdata_t *gd, result_t *result, int depth, int bound, int best_move)
{
	assert(best_move >= 0);

	void *pos = &(gd->pos);
	uint32_t *hash = gdata_get_hash(gd);
	int search_depth = iddfs_depth - depth;

	trans_value_t value =
	{
		.score = result->score,
		.full = result->full,
		.bound = bound,
		//.iddfs = iddfs_depth,
		//.depth = depth,
		.search_depth = search_depth,
		.best_move = best_move,
	};


	return hashmap_add_kvpair(trans_tbl, pos, &value, hash);
}

//trans_value_t *tt_get(gdata_t *gd, int depth)
bool tt_get(trans_value_t *value, gdata_t *gd, int depth)
{
	void *pos = &(gd->pos);
	uint32_t *hash = gdata_get_hash(gd);
	//trans_value_t *value = hashmap_key_get_value(trans_tbl, pos, hash);
	bool got = hashmap_key_get_value(trans_tbl, pos, value, hash);

	//if(!value && solver->flip && depth<=solver->flip_depth)
	if(!got && solver->flip && depth<=solver->flip_depth)
	{
		uint8_t flipped[solver->pos_size];
		solver->flip(flipped, pos);
		//value = hashmap_key_get_value(trans_tbl, flipped, NULL);
		got = hashmap_key_get_value(trans_tbl, flipped, value, NULL);
	}


	//return value;
	return got;
}

/*bool alphabeta_cutoff(float cscore, float prune,
	float *best_so_far, int depth)
{
	if(is_worse(cscore, prune, depth))
	{
		return true;
	}
	if(is_better(cscore, *best_so_far, depth))
	{
		*best_so_far = cscore;
	}
	return false;
}*/

bool is_better(float s0, float s1, int depth)
{
	float dif = s0 - s1;
	bool p0_turn = max_or_min(depth);
	if(!p0_turn)
		dif *= -1;
	return dif > 0;
}

bool is_worse(float s0, float s1, int depth)
{
	float dif = s0 - s1;
	bool p0_turn = max_or_min(depth);
	if(!p0_turn)
		dif *= -1;
	return dif < 0;
}

float worst_score(int depth)
{
	float worst = (max_or_min(depth)==MAX_LAYER)? -WIN_SCORE-1 : WIN_SCORE+1;
	//float worst = max_or_min(depth)? -1 : 1;
	assert(is_worse(worst, 0, depth));
	return worst;
}

bool is_win_score(float s, int depth)
{
	bool turn = max_or_min(depth)==MAX_LAYER;
	if(turn)
		return s > MATE_LIMIT;
	else
		return s < -MATE_LIMIT;
}

float max(float x, float y)
{
	return (x>y)? x : y;
}

float min(float x, float y)
{
	return (x<y)? x : y;
}

float abs_f(float a)
{
	return (a>=0)? a : -a;
}

bool time_up(void)
{
	#ifndef FORCE_SEARCH_DEPTH
	bool clock_flag = clock_update();
	if(clock_flag)
	{
		return true;
	}
	#endif

	if(time_lim == 0)
		return false;
	else
		return (toc_ms() >= time_lim);
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


bool max_or_min(int depth)
{
	bool mm = !(depth & 0b1);
	if(!who_goes_first)
		mm = !mm;
	return mm? MAX_LAYER : MIN_LAYER;
}



/*tnode_t *node_make_new_move(tree_t *gt, tnode_t *n, int move)
{
	tnode_t *child = NULL;
	//if(solver->is_legal(node_get_pos(n), move))
	if(1)
	{
		tree_get(gt, n);
		tree_add_copies(gt, 1);
		//tree_add(gt, NULL);
		child = n->children[n->child_ct-1];

		uint32_t *hp = node_get_hash(child);
		// *hp = *node_get_hash(n);


		solver->make_move(node_get_pos(child), move, hp);
		child->move_index = move;
		//assert(child->score == 0);


	}
	return child;
}*/

bool make_new_move(gdata_t *child, gdata_t *gd, int move)
{
	/*tree_get(gt, n);
	tree_add_copies(gt, 1);
	//tree_add(gt, NULL);
	child = n->children[n->child_ct-1];*/

	memcpy(child, gd, gdata_size);

	uint32_t *hp = gdata_get_hash(child);
	solver->make_move(&(child->pos), move, hp);
	child->move_index = move;

	return true;	//always?
}

/*void *node_get_pos(tnode_t *n)
{
	gdata_t *d = n->data;
	return &(d->pos);
}

uint32_t *node_get_hash(tnode_t *n)
{
	if(solver->uses_zobrist)
	{
		gdata_t *d = n->data;
		return &(d->hash);
	}
	else
		return NULL;
}*/

uint32_t *gdata_get_hash(gdata_t *gd)
{
	if(solver->uses_zobrist)
		return &(gd->hash);
	else
		return NULL;
}
