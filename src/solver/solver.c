

#include "solver.h"

#include "../utils/utils.h"

#include "transposition.h"
#include "ui/play_windows.h"
#include "clock.h"
#include "zobrist.h"
#include "ui/solver_ui.h"
#include "shared.h"

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

//statics
bool set_aspiration_window(float *asp_window,
	float *asp_window_size, float *last_score, float score,
	bool verbose);
result_t eval(gdata_t *gd, int depth,
	float alpha, float beta, bool is_pv);
int build_movelist(sorter_t *order, void *pos);
int sort_movelist(sorter_t *order, int len,
	gdata_t *gd, int depth, trans_value_t *vp);
bool move_is_forcing(void *pos, int move);
result_t analyze_all_children(gdata_t *gd, trans_value_t *ttval,
	sorter_t *order, int len, int depth, float alpha, float beta,
	bool is_pv);
//bool alphabeta_cutoff(float cscore, float prune,
//	float *best_so_far, int depth);
float max(float x, float y);
float min(float x, float y);
bool time_up(void);


bool is_better(float s0, float s1, int depth);
bool is_worse(float s0, float s1, int depth);
float worst_score(int depth);
bool is_win_score(float s, int depth);
bool max_or_min(int depth);

int FORCE_SEARCH_DEPTH = 0;	//declared extern in solver.h
//solver_t *solver;
bool who_goes_first = true;
uint32_t position_ct = 0;
int iddfs_depth;
int time_lim;
int full_solve_depth = 0;
bool main_thread_done = false;

int *HISTORY_VALS;
//uint8_t HISTORY_BEST[2] = {-1, -1};

#define MAX_PLY	(42)
#define NUM_KILLERS	(2)
uint8_t killers[MAX_PLY][NUM_KILLERS] = {0};

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
	if(solver->init)
		solver->init();
	//if(!solver->hash_size)
	//	solver->hash_size = solver->pos_size;
	gdata_size = sizeof(gdata_t) + solver->pos_size;
	position_ct = 0;

	#ifdef USE_TRANSPOSITION_TABLE
	//tt_create(solver->transtbl_buckets_ct);
	tt_create();
	#endif	//USE_TRANSPOSITION_TABLE
}

void solver_clear(void)
{
	tt_destroy();
	zobrist_free();
}

float solve(solver_t *game_solver, void *pos,
	int time_lim_ms, bool verbose)
{
	//tic();

	time_lim = time_lim_ms;
	position_ct = 0;

	if(!pos)
		pos = solver->initial_pos;
	gdata_t *gd = mem_malloc(gdata_size);
	gd->hash = solver->hash(pos, solver->pos_size);
	memcpy(&(gd->pos), pos, solver->pos_size);




	who_goes_first = solver->whosemove(pos);

	if(solver->moves_remaining)
		full_solve_depth = solver->moves_remaining(pos);
	//printf("full solve depth = %d\n", full_solve_depth);

	if(verbose)
	{
		printf("solving ");
		if(FORCE_SEARCH_DEPTH)
			printf("(force search depth %d)", FORCE_SEARCH_DEPTH);
		else
			printf("(time limit %d ms)", time_lim_ms);
		printf("\n");

		//printf("pre setup time = %d ms\n", toc_ms());
	}

	tic();

	#ifdef USE_HISTORY_HEURISTIC
	int history_scores[2 * solver->possible_moves];
	for(int i=0; i<2 * solver->possible_moves; i++)
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
		//if we're almost at full depth, just go for it
		if(solver->moves_remaining)
		{
			int remaining = solver->moves_remaining(gd->pos);
			if(remaining - iddfs <= incr/2)
				iddfs = remaining;
		}

		//clamp to given max depth
		if(FORCE_SEARCH_DEPTH)
		{
			if(iddfs > FORCE_SEARCH_DEPTH)
				iddfs = FORCE_SEARCH_DEPTH;
		}

		iddfs_depth = iddfs;

		uint32_t last = toc_ms();

		//tt_set_ancient();


		float asp_window_size[] = {solver->aspiration_default_width,
								solver->aspiration_default_width};
		(void)asp_window_size;
		(void)last_iddfs_score;
		while(1)
		{
			if(verbose)
				printf("\niddfs=%d in window [%.1f,%.1f] ",
					iddfs, asp_window[0], asp_window[1]);

			if(MULTICORE_CT > 1)
			{
				main_thread_done = false;

				#pragma omp parallel for private(solver)
				//#pragma omp parallel for
				for(int mp=0; mp<MULTICORE_CT; mp++)
				{
					int tid = omp_get_thread_num();
					//printf("solving from thread %d\n", tid);
					solver = (tid==0)? game_solver : &alt_solvers[tid-1];
					result = eval(gd, 0,
						asp_window[0], asp_window[1],
						true);

					if(tid==0)
						main_thread_done = true;
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
				result.score, verbose);
			#endif
			if(in_window)
				break;
		}

		if(time_up())
		{
			result = last_result;
			if(verbose) printf("(time up)\n");
			break;
		}


		last_result = result;
		iddfs_complete = iddfs;

		//print info
		if(verbose)
		{
			print_evaluation(gd, iddfs_depth, result);

			uint32_t now = toc_ms();
			window_focus(analysis_hdl);
			printf("(%u ms)", now-last);
			last = now;
		}
		//printf("      ");
		//window_focus(analysis_hdl);



		//conditions for ending the search
		if(result.full)
			break;

		if(FORCE_SEARCH_DEPTH)
			if(iddfs >= FORCE_SEARCH_DEPTH)
				break;

		#ifdef USE_HISTORY_HEURISTIC
		for(int i=0; i<2*solver->possible_moves; i++)
			HISTORY_VALS[i] /= 2;
		#endif
	}

	//count time
	uint32_t us = toc();
	uint32_t ms = us/1000;
	uint32_t sec_total = ms/1000;
	uint32_t min = sec_total/60;
	uint32_t sec = sec_total - (60*min);

	if(result.best_move == -1)
		result.best_move = random_move(gd->pos);
	int best_move = result.best_move;

	/*int best_move = result.best_move;
	//assert(best_move != -1);
	if(best_move == -1)
		best_move = random_move(gd->pos);*/

	if(verbose)
	{
		printf("\n\n--- search ran to depth = %d%s ---\n",
			iddfs_complete, result.full? " (full solve)":"");


		printf("best move: \t");
		if(solver->iter_to_human)
			printf("%s\n", solver->iter_to_human(best_move));
		else
			printf("%d\n", best_move);
		printf("evaluation:\t%+.1f\n", result.score);

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
		#ifdef USE_TRANSPOSITION_TABLE
		//printf("hashmap load factor = %d%%\n", tt_load());
		//printf("number of collisions: %s\n", sprintbig(tt_collisions(), "%d"));
		#endif
	}


	//clean up
	mem_free(gd);


	//term_move_cursor(0, 15);

	#ifdef USE_HISTORY_HEURISTIC
	window_unfocus();
	term_move_cursor(0, 36);
	for(int r=0; r<7; r++)
	{
		int space = printf("%d:%d   ",
			r, HISTORY_VALS[r]/1000);
		for(int i=0; i<10-space; i++)
			putchar(' ');
	}
	printf("\n\n");
	for(int r=0; r<7; r++)
	{
		int space = printf("%d:%d   ",
			r, HISTORY_VALS[r + solver->possible_moves]/1000);
		for(int i=0; i<10-space; i++)
			putchar(' ');
	}
	#endif	//USE_HISTORY_HEURISTIC

	return best_move;
	//return result;
}

//returns true if we're in the window
bool set_aspiration_window(float *asp_window,
	float *asp_window_size, float *last_score, float score,
	bool verbose)
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

		if(verbose)
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
		return (result_t){.score=0, .full=false, .has_tt=false, .best_move=-1};


	//if(is_pv)
	//	printf("\tpv node at d=%d w [%.1f,%.1f]\n",
	//	depth, alpha, beta);
	assert(gd);
	if(depth > 1000)
	{
		printf("error! depth is crazy big lol\n");
		exit(0);
	}
	void *pos = &(gd->pos);



	//trans_value_t *ttval = NULL;
	trans_value_t ttval;
	bool got = false;
	#ifdef USE_TRANSPOSITION_TABLE
	//check if position was already analyzed
	//tt_prefetch(gd->hash);
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
			return (result_t){.score=gd->score, .full=true, .has_tt=true, .best_move=ttval.best_move};
		}
		//if(ttval->iddfs >= iddfs_depth && !asp_window_rerun)
		//if(ttval->iddfs >= iddfs_depth && ttval->bound==BOUND_EXACT)
		//assert(!ttval->full);
		if(ttval.search_depth >= (iddfs_depth - depth)
			&& ttval.bound==BOUND_EXACT)
		//if(val->iddfs >= iddfs_depth
		//	&& !(asp_window_rerun && !val->exact))
			return (result_t){.score=gd->score, .full=false, .has_tt=true, .best_move=ttval.best_move};
	}
	#endif

	//update metrics
	//if(!(position_ct & 0xFFFFF))	//every few 100k
	//	draw_tt_load();
	position_ct++;

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
		return (result_t){.score=gd->score, .full=true, .has_tt=false, .best_move=-1};
	}
	else if(depth >= iddfs_depth)
	{
		//quiescence search?
		/*bool quiescence_extend = false;
		if(iddfs_depth < full_solve_depth)
		{
			sorter_t ml[solver->possible_moves];
			int test_len = 0;

			if(solver->only_moves)
				test_len = solver->only_moves(ml, pos);
			if(test_len)	//forced moves - pos is not quiet
			{
				depth -= 2;
				quiescence_extend = true;
			}
		}


		if(!quiescence_extend)
		{*/
		if(solver->estimate)
			gd->score = solver->estimate(pos);
		else
			gd->score = 0;
		assert(-MATE_LIMIT < gd->score && gd->score < MATE_LIMIT);
		return (result_t){.score=gd->score, .full=false, .has_tt=false, .best_move=-1};
		//}
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

		len = sort_movelist(movelist, len, gd, depth,
			got? &ttval : NULL);
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
	tt_add(gd->pos, gdata_get_hash(gd), &result,
		iddfs_depth-depth, bound, result.best_move);
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
	assert(sdq > 0);

	//apply bonuses/maluses
	int off = solver->whosemove(pos)? 0 : solver->possible_moves;
	//for(int i=0; i<=index; i++)
	for(int i=0; i<len; i++)
	{
		int placement = order[i].move + off;
		assert(placement < 2 * solver->possible_moves);

		if(i == index)
			HISTORY_VALS[placement] += sdq;
		else
			HISTORY_VALS[placement] -= sdq;
	}

	//if(index == 0)
	//	printf("hey!\n");

	return;
}

int get_lmr_reduction(int i, int depth, bool is_pv)
{
	//return 0;

	//const int lmr_start = 1;
	const int candidates = 2;
	const int lmr_min_depth = 4;

	int reduction = 0;
	int late = i - candidates + 1;
	if(!is_pv && late > 0
		&& lmr_min_depth <= depth
		&& ((iddfs_depth < full_solve_depth) || !full_solve_depth)
	)
	{
		reduction = 2*(late/2 + 1);

		//if(reduction >= (iddfs_depth - depth))
		//	reduction = iddfs_depth - depth - 1;
	}
	assert(reduction >= 0);
	assert(!(reduction & 0b1));

	return reduction;
}

result_t analyze_all_children(gdata_t *gd, trans_value_t *ttval,
	sorter_t *order, int len, int depth, float alpha, float beta,
	bool is_pv)
{

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
		//gdata_t child;
		//gdata_t *child = sl_alloc(gdata_size);
		uint8_t child[gdata_size];
		bool made = make_new_move((gdata_t *)&child, gd, move);
		//if(!child)
		if(!made)
		{
			assert(0);
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
		//result = eval((gdata_t *)&child, depth+1,
		//	alpha, beta, child_pv);

		/*int extension = 0;
		if(solver->get_extension)
			extension = solver->get_extension(gd->pos);

		if(extension)
		{
			result = eval((gdata_t *)&child, depth+1-extension,
				alpha, beta, child_pv);
		}
		else	//reduction
		{*/
			int reduction = get_lmr_reduction(i, depth, is_pv);
			result = eval((gdata_t *)&child, depth+1+reduction,
				alpha, beta, child_pv);
			if(reduction)
			{
				if((is_max && result.score > alpha)
					|| (!is_max && result.score < beta))
				{
					//redo without reduction
					reduction = 0;
					result = eval((gdata_t *)&child, depth+1+reduction,
						alpha, beta, child_pv);
				}
			}
		//}

		//sl_free(child);

		if(multi_pv && !result.full)
			multi_full = false;

		#ifdef RETURN_FIRST_WIN_FOUND
		if(is_win_score(result.score, depth))
		{
			best_result = result;
			best_result.best_move = move;

			/*#ifdef USE_HISTORY_HEURISTIC
			if(!result.has_tt)
				update_history(gd->pos, i, order, len, depth, true);
			#endif*/

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
			//assert(alpha+1 == beta);
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
			if(!result.has_tt && len<solver->possible_moves)
				update_history(gd->pos, i, order, len, depth, true);
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
	//assert(is_win_score(best, depth) == best_full);

	//n->score = best;	//fail soft

	//in multi pv, all nodes need to fully evaluate to depth
	if(multi_pv && !multi_full)
		best_result.full = false;

	//return (result_t){.score=best, .full=best_full, .best_move=best_move};
	best_result.has_tt = false;
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


int sort_movelist(sorter_t *order, int len, gdata_t *gd, int depth,
	trans_value_t *vp)
{
	void *pos = &(gd->pos);


	//trans_value_t v;
	//bool got = tt_get(&v, gd, depth);
	//int best = got? v.best_move : -1;
	int best = vp? vp->best_move : -1;

	uint8_t *killers_ply = killers[depth];
	(void)killers_ply;


	#ifdef USE_HISTORY_HEURISTIC
	int history_best=-1, history_thresh=-100000000;
	int off = solver->whosemove(gd->pos)? 0 : solver->possible_moves;
	//int history_second;
	//bool history_set = false;
	for(int i=0; i<solver->possible_moves; i++)
	{
		if(HISTORY_VALS[i+off] > history_thresh)
		{
			history_best = i;
			history_thresh = HISTORY_VALS[i+off];
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
			order[i].score += (1<<16);
		#ifdef USE_HISTORY_HEURISTIC
		else if(move == history_best)
			order[i].score += (1<<14);
		//else if(move == history_second)
		//	order[i].score += 600;
		#endif	//USE_HISTORY_HEURISTIC
		else if(move_is_forcing(pos, move))
			order[i].score += (1<<15);



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

void catch_pos(void *pos)
{
	window_unfocus();
	term_move_cursor(0, 12);
	solver->draw_full(pos, -1);
	printf("\n(caught pos, press any key)");
	getchar();
	window_focus(analysis_hdl);
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

bool time_up(void)
{
	if(!FORCE_SEARCH_DEPTH)
	{
		bool clock_flag = clock_update();
		if(clock_flag)
		{
			return true;
		}
	}

	if(main_thread_done && omp_get_thread_num())
		return true;

	if(time_lim == 0)
		return false;
	else
		return (toc_ms() >= time_lim);
}


bool max_or_min(int depth)
{
	bool mm = !(depth & 0b1);
	if(!who_goes_first)
		mm = !mm;
	return mm? MAX_LAYER : MIN_LAYER;
}
