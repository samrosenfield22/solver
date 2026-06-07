

#include "solver.h"

#include "../../utils.h"

#include "play_windows.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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
} result_t;

//statics
void tt_create(void);
int tt_add(tnode_t *n, result_t *result, int depth,
	int bound, int best_move);
bool set_aspiration_window(float *asp_window,
	float *asp_window_size, float *last_score, float score);
result_t eval(tree_t *gt, tnode_t *n, int depth,
	float alpha, float beta, bool is_pv);
//int build_order(sorter_t *order, tree_t *gt, tnode_t *n, int depth);
int build_movelist(sorter_t *order, void *pos);
void sort_movelist(sorter_t *order, int len,
	tree_t *gt, tnode_t *n, int depth);
bool move_is_forcing(void *pos, int move);
void add_all_new_moves(tree_t *gt, tnode_t *n, int depth);
void order_children(tree_t *gt, tnode_t *n, int depth);
float sort_score(tree_t *gt, tnode_t *parent, tnode_t *n,
	int best, int depth, int index);
result_t analyze_all_children(tree_t *gt, tnode_t *n, trans_value_t *ttval,
	sorter_t *order, int len, int depth, float alpha, float beta,
	bool is_pv);

bool tt_check(tnode_t *n);
trans_value_t *tt_get(tnode_t *n, int depth);

bool alphabeta_cutoff(float cscore, float prune,
	float *best_so_far, int depth);
//void evaluate(tree_t *gt, solver_t *solver);
float max(float x, float y);
float min(float x, float y);
void print_score(float score);
void minimax(tree_t *gt, int depth);
void clear_suboptimal_nodes(tree_t *gt, tnode_t *n, int depth, int var_length);

bool is_better(float s0, float s1, int depth);
bool is_worse(float s0, float s1, int depth);
float worst_score(int depth);
bool is_win_score(float s, int depth);
bool max_or_min(int depth);

tnode_t *node_make_new_move(tree_t *gt, tnode_t *n, int move);
void *node_get_pos(tnode_t *n);
uint32_t *node_get_hash(tnode_t *n);

solver_t *solver;
hashmap_t *trans_tbl = NULL;
bool who_goes_first = true;
uint32_t position_ct = 0, max_node_ct = 0;
int iddfs_depth;

typedef struct
{
	uint32_t hash;
	uint8_t pos[];
} gdata_t;
size_t gdata_size, gdata_hash_size;

int *HISTORY_VALS;
uint8_t HISTORY_BEST[2] = {-1, -1};

#define MAX_PLY	(42)
#define NUM_KILLERS	(2)
uint8_t killers[MAX_PLY][NUM_KILLERS] = {0};



//uint32_t passed=0, checked=0;

void print_eval_bar(float score)
{


	window_focus(eval_hdl);
	//window_clear();
	window_cursor_set(0);

	int len = 40;
	char *indent = "  ";

	printf(indent);
	for(int i=0; i<len/2-1; i++)
		printf(" ");
	print_score(score);
	printf("\n");

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

	window_focus(analysis_hdl);
}

/*void *construct_pos(solver_t *game_solver, char *seq)
{
	if(!seq)
		return NULL;
	printf("got seq: \'%s\'\n", seq);

	solver = game_solver;

	void *pos = mem_malloc(solver->pos_size);
	memcpy(pos, solver->initial_pos, solver->pos_size);

	for(char *token = strtok(seq, ","); token;
		token = strtok(NULL, ","))
	{
		if(*token == '\n')
			break;
		int move = strtol(token, NULL, 10);
		printf("move=%d\n", move);

		if(solver->is_legal(pos, move))
			solver->make_move(pos, move, NULL);
		else
		{
			printf("illegal move %d\n", move);
			return NULL;
		}
	}

	return pos;
}*/

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


}

float solve(solver_t *game_solver, void *pos, int init_depth,
	int time_lim_ms, bool verbose)
{
	solver_check(game_solver);

	solver = game_solver;

	if(!solver->hash_size)
		solver->hash_size = solver->pos_size;

	position_ct = 0;
	max_node_ct = 0;


	//make the tree
	gdata_size = sizeof(gdata_t) + solver->pos_size;
	//tree_t *gt = tree_create(solver->pos_size);
	tree_t *gt = tree_create(gdata_size);
	tree_clear_search_depth(gt);
	tree_attach_print_fn(gt, solver->print_pos);
	#ifdef USE_TRANSPOSITION_TABLE
	if(!trans_tbl)
		tt_create();
	#endif	//USE_TRANSPOSITION_TABLE

	if(!pos)
		pos = solver->initial_pos;
	//tree_add(gt, pos);
	gdata_t *th = mem_malloc(gdata_size);
	th->hash = solver->hash(pos, solver->pos_size);
	memcpy(&th->pos, pos, solver->pos_size);
	tree_add(gt, th);
	mem_free(th);

	//term_clear();

	//window_focus(eval_hdl);
	//printf("\n\n\n\n");
	window_focus(analysis_hdl);
	window_clear();


	who_goes_first = solver->whosemove(pos);
	printf("player %d to move\n", who_goes_first? 1 : 2);
	//exit(0);



	if(solver->print_pos)
	{
		printf("solving game position: ");
		solver->print_pos(node_get_pos(gt->head));
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

	tic();

	#ifdef USE_HISTORY_HEURISTIC
	int history_scores[solver->possible_placements];
	for(int i=0; i<solver->possible_placements; i++)
		history_scores[i] = 0;
	HISTORY_VALS = history_scores;
	#endif

	result_t result;
	float asp_window[] = {-WIN_SCORE, WIN_SCORE};
	float last_iddfs_score = 0;

	//iddfs
	int iddfs;
	int incr = solver->iddfs_increment;
	if(!incr)
		incr = 1;
	for(iddfs=0;; iddfs+=incr)
	{
		//iddfs_depth = iddfs + init_depth;
		iddfs_depth = iddfs;

		//if(verbose)
		//	printf("\niddfs depth=%d\n", iddfs);

		tree_set_search_depth(gt, iddfs);
		uint32_t last = toc_ms();


		float asp_window_size[] = {solver->aspiration_default_width,
								solver->aspiration_default_width};
		(void)asp_window_size;
		(void)last_iddfs_score;
		while(1)
		{

			printf("\niddfs=%d in window [%.1f,%.1f] ",
				iddfs, asp_window[0], asp_window[1]);
			result = eval(gt, gt->head, 0,
				asp_window[0], asp_window[1],
				true);


			bool in_window = true;
			#ifdef ASPIRATION_WINDOW
			in_window = set_aspiration_window(asp_window,
				asp_window_size, &last_iddfs_score,
				result.score);
			#endif
			if(in_window)
				break;

			//asp window failed
			//clear the tree
			tnode_t *h = tree_get(gt, gt->head);
			while(h->child_ct)
				tree_delete_child(gt, 0);
		}

		//print info
		if(verbose)
		{
			window_focus(eval_hdl);
			//window_clear();
			window_cursor_set(0);
			print_eval_bar(result.score);
			window_focus(eval_hdl);
			if(iddfs >= 1)
			{
				//window_cursor_set(4);
				printf("\t\t   (depth: %d)\n\n", iddfs);
				printf("  %+.2f   ", result.score);
				tnode_t *n = gt->head->children[0];
				for(int i=0; i<2*VARIATION_LENGTH; i++)
				{
					if(i)
						printf(", ");
					if(i >= iddfs)
						break;
					if(!n)
						break;

					if(solver->iter_to_human)
						printf(solver->iter_to_human(n->move_index));
					else
						printf("%d", n->move_index);

					if(!n->child_ct)
						break;
					//assert(n->child_ct);
					n = n->children[0];
				}

				uint32_t now = toc_ms();
				window_focus(analysis_hdl);
				printf("(%u ms)", now-last);
				window_focus(eval_hdl);
				last = now;
			}
		}
		printf("      ");
		window_focus(analysis_hdl);

		//conditions for ending the search
		if(result.full)
			break;

		#ifdef FORCE_SEARCH_DEPTH
			if(iddfs >= FORCE_SEARCH_DEPTH)
				break;
		#else
			if((toc_ms() >= time_lim_ms) && !(iddfs & 0b1))
				break;
		#endif

		#ifdef USE_HISTORY_HEURISTIC
		for(int i=0; i<solver->possible_placements; i++)
			HISTORY_VALS[i] /= 2;
		#endif

		//clear the tree
		tnode_t *h = tree_get(gt, gt->head);
		while(h->child_ct)
			tree_delete_child(gt, 0);
	}

	//count time
	uint32_t us = toc();
	uint32_t sec_total = us/1000000;
	uint32_t min = sec_total/60;
	uint32_t sec = sec_total - (60*min);

	int best_move = gt->head->children[0]->move_index;

	if(verbose)
	{
		//output
		//printf("\n\n");
		//tree_draw(gt, VARIATION_LENGTH*2);

		printf("\n\n--- search ran to depth = %d%s ---\n", iddfs,
			result.full? " (full solve)":"");

	}

	printf("best move: \t\t");
	if(solver->iter_to_human)
		printf("%s\n", solver->iter_to_human(best_move));
	else
		printf("%d\n", best_move);
	printf("evaluation: %.1f\n", result.score);
	//print_eval_bar(gt->head->children[0]->score);
	//printf("\n\n");

	if(verbose)
	{
		//printf("eval: %+.1f\n", gt->head->children[0]->score);
		printf("\n\nposition solved in %d m, %d sec\n", min, sec);
		printf("time per position: %.2f us\n", ((float)us)/position_ct);
		printf("evaluated %s unique positions\n", sprintbig(position_ct, "%d"));
		printf("greatest number of nodes stored in tree: %u\n", max_node_ct);
		#ifdef USE_TRANSPOSITION_TABLE
		printf("hashmap load factor = %d%%\n", hashmap_load(trans_tbl));
		printf("number of collisions: %s\n", sprintbig(hashmap_collisions(trans_tbl), "%d"));
		#endif
	}

	//tree_clear(gt);
	tree_destroy(gt);
	hashmap_destroy(trans_tbl);
	zobrist_free();
	trans_tbl = NULL;

	term_move_cursor(0, 15);

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
result_t eval(tree_t *gt, tnode_t *n, int depth,
	float alpha, float beta, bool is_pv)
{
	//if(is_pv)
	//	printf("\tpv node at d=%d w [%.1f,%.1f]\n",
	//	depth, alpha, beta);
	assert(n);
	//assert(n->child_ct == 0);
	if(depth > 1000)
	{
		printf("error! depth is crazy big lol\n");
		exit(0);
	}
	void *pos = node_get_pos(n);

	#ifdef USE_TRANSPOSITION_TABLE
	//check if position was already analyzed
	trans_value_t *ttval = tt_get(n, depth);
	//if(val && (val->iddfs == iddfs))
	//	return n->score;
	if(ttval)
	{
		n->score = ttval->score;
		//assert(ttval->score == n->score);
		//if(n->score > MATE_LIMIT || n->score < -MATE_LIMIT)
		if(ttval->full)
		{
			assert(n->score > MATE_LIMIT
				|| n->score < -MATE_LIMIT
				|| n->score == 0);
			return (result_t){n->score, true};
		}
		//if(ttval->iddfs >= iddfs_depth && !asp_window_rerun)
		if(ttval->iddfs >= iddfs_depth && ttval->bound==BOUND_EXACT)
		//if(val->iddfs >= iddfs_depth
		//	&& !(asp_window_rerun && !val->exact))
			return (result_t){n->score, false};
	}
	#endif

	//update metrics
	position_ct++;
	if(gt->node_ct > max_node_ct)
		max_node_ct = gt->node_ct;


	//evaluate end states/leaf nodes
	int endstate = solver->gameover(pos);
	if(endstate != END_NOT_OVER)
	{
		switch(endstate)
		{
			case END_P1_WON:	n->score = WIN_SCORE;	break;
			case END_P2_WON:	n->score = -WIN_SCORE;	break;
			case END_DRAW:		n->score = 0;			break;
			default:	printf("invalid endstate!\n"); exit(0);
		}
		return (result_t){n->score, true};
	}
	else if(depth >= gt->search_depth)
	{
		if(solver->estimate)
			n->score = solver->estimate(pos);
		else
			n->score = 0;
		assert(-WIN_SCORE < n->score && n->score < WIN_SCORE);
		return (result_t){n->score, false};
	}

	//float score;

	//make move order
	//if there's only one move (that wins or loses),
	//just play that
	sorter_t movelist[solver->possible_moves];
	int len = 0;

	if(solver->only_moves)
		len = solver->only_moves(movelist, pos);

	if(!len)
	{
		assert(n->child_ct == 0);

		if(solver->make_movelist)
			len = solver->make_movelist(movelist, pos);
		else
			len = build_movelist(movelist, pos);

		sort_movelist(movelist, len, gt, n, depth);
		//len = build_order(order, gt, n, depth);
		assert(n->child_ct == 0);

		//add_all_new_moves(gt, n, depth);
		//order_children(gt, n, depth);
		//int *order;

		//len = solver->possible_moves;
	}

	//main analysis -- recursive tree search
	result_t result = analyze_all_children(gt, n, ttval, movelist, len,
		depth, alpha, beta, is_pv);

	//assert(n->child_ct);
	//int best_move = n->children[0]->move_index;
	if(is_win_score(result.score, depth))
	{
		result.score -= (max_or_min(depth)==MAX_LAYER)? 1: -1;
		n->score = result.score;
	}
	int best_move = n->child_ct? n->children[0]->move_index : -1;

	#ifdef CLEAR_SUB_NODES
	//clear nodes except the best one(s)
	clear_suboptimal_nodes(gt, n, depth, VARIATION_LENGTH);
	#endif

	#ifdef USE_TRANSPOSITION_TABLE
	int bound;
	if(alpha < result.score
		&& result.score < beta)
		bound = BOUND_EXACT;
	else if(result.score < alpha)
		bound = BOUND_UPPER;
	else
		bound = BOUND_LOWER;
	tt_add(n, &result, depth, bound, best_move);
	#endif

	assert(result.score == n->score);
	//return n->score;
	return result;
}

void add_all_new_moves(tree_t *gt, tnode_t *n, int depth)
{
	/*
	for each child that's already there,
	make sure tt score == child score
	*/

	/*if(depth+1 != iddfs)
	for(int i=0; i<n->child_ct; i++)
	{
		checked++;
		tnode_t *child = n->children[i];
		trans_value_t *val = tt_get(child);
		if(val)
		{
			assert(val->score == child->score);
			if(!(val->depth == depth+1))
				printf("val->depth=%d, depth+1=%d\n", val->depth, depth+1);
			assert(val->depth == depth+1);
			//printf("passed!\n");
			passed++;
		}
		else
			if(depth < 5)
				printf("no tt record for d=%d\n", depth+1);
	}*/

	bool already_made[solver->possible_moves];
	for(int i=0; i<solver->possible_moves; i++)
		already_made[i] = false;
	for(int i=0; i<n->child_ct; i++)
		already_made[n->children[i]->move_index] = true;

	for(int i=0; i<solver->possible_moves; i++)
	{
		int move = solver->default_order?
			solver->default_order[i] : i;

		//check if move already has a node
		if(already_made[move])
			continue;
		already_made[move] = true;

		tree_get(gt, n);	//do i need?
		node_make_new_move(gt, n, move);
	}

	assert(n->child_ct <= solver->possible_moves);

}

void order_children(tree_t *gt, tnode_t *n, int depth)
{
	/*printf("before sorting @ d=%d:\n", depth);
	for(int i=0; i<n->child_ct; i++)
		printf("%.2f (m%d), ", n->children[i]->score, n->children[i]->move_index);
	printf("\n");*/

	trans_value_t *val = tt_get(n, depth);
	int best = val? val->best_move : -1;

	//assign sort scores
	tree_get(gt, n);
	for(int i=0; i<n->child_ct; i++)
	{
		n->children[i]->score = sort_score(gt, n, n->children[i],
			best, depth, i);
	}

	tree_get(gt, n);
	//bool order = max_or_min(depth);
	tree_attach_compare_fn(gt, compare_by_score_ascending);
	//
	//sort the first n children (those with transtab values)
	//qsort(n->children, stored, sizeof(void*), gt->compare_fp);

	tree_sort_children(gt);

	//clear sort scores
	for(int i=0; i<n->child_ct; i++)
		n->children[i]->score = 0;

	//for(int i=0; i<n->child_ct; i++)
	//	printf("%d, ", n->children[i]->move_index);
	//printf("\n");
}

float sort_score(tree_t *gt, tnode_t *parent, tnode_t *n,
	int best, int depth, int index)
{
	float score;


	//int end = solver->gameover(n);
	//int win = max_or_min(depth)? END_P1_WON : END_P2_WON;

	/*if(end == win)
		score = 1000;
	else *//*if(n->move_index == best)
		score = 999;
	else*/
	{
		score = solver->estimate_sort(node_get_pos(n), n->move_index);
		/*trans_value_t *val = tt_get(n);
		if(val)
		{
			score = val->score;
			if(max_or_min(depth)==MIN_LAYER)
				score *= -1;
		}
		else
			score = solver->possible_moves - index;*/

		//tiebreak
		score -= (float)index/20;
		//float defaults = solver->possible_moves - index;
		//score += defaults/20;
	}

	return score;
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

void update_history(tnode_t *n, int index,
	sorter_t *order, int len, int depth, bool was_cutoff)
{
	void *pos = node_get_pos(n);
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

result_t analyze_all_children(tree_t *gt, tnode_t *n, trans_value_t *ttval,
	sorter_t *order, int len, int depth, float alpha, float beta,
	bool is_pv)
{
	//assert(n->child_ct == 0);

	if(ttval && ttval->iddfs >= iddfs_depth)
	{
		if(ttval->bound == BOUND_LOWER)
			alpha = max(alpha, ttval->score);
		else if(ttval->bound == BOUND_UPPER)
			beta = min(beta, ttval->score);
	}
	if(alpha >= beta)	//bail immediately
	{
		assert(ttval->full == false);
		return (result_t){ttval->score, false};
	}

	float alpha_init = alpha, beta_init = beta;
	(void)alpha_init;
	(void)beta_init;

	result_t result;
	float best = worst_score(depth);
	bool best_full = false;
	int best_index = 0;
	bool is_max = (max_or_min(depth)==MAX_LAYER);

	//for(int i=0; i<n->child_ct; i++)
	for(int i=0; i<len; i++)
	{
		tree_get(gt, n);

		int move = order[i].move;
		assert(move != -1);
		if(move == -1)
			continue;
		assert(0 <= move && move < solver->possible_moves);
		tnode_t *child = node_make_new_move(gt, n, move);
		//assert(child);	//if false, the move wasn't legal
		if(!child)
		{
			assert(len != 1);
			//assert(i);
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
		result = eval(gt, child, depth+1,
			alpha, beta, child_pv);



		if(is_win_score(result.score, depth))
		{
			best_full = true;

			#ifdef RETURN_FIRST_WIN_FOUND
			best = result.score;
			best_index = n->child_ct-1;

			#ifdef USE_HISTORY_HEURISTIC
			update_history(n, i, order, len, depth, true);
			#endif

			goto analyze_end;
			#endif	//RETURN_FIRST_WIN_FOUND
		}


		if(is_better(result.score, best, depth))
		{
			best = result.score;
			best_index = n->child_ct-1;
		}

		//if(!result.full)
		//	best_full = false;

		#ifdef USE_ALPHABETA_PRUNING

		#ifdef PRINCIPAL_VAR_SEARCH
		if(is_pv && i)
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

		if(is_max)
			alpha = max(alpha, result.score);
		else	//min
			beta = min(beta, result.score);

		if(alpha >= beta)
		{
			//update history
			#ifdef USE_HISTORY_HEURISTIC
			update_history(n, i, order, len, depth, true);
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

			//if(!(depth==0 && i<PRINCIPAL_VAR_CT))
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
			if(is_max)
				beta = alpha+1;
			else
				alpha = beta-1;
			//printf("[%.1f,%.1f]\n", alpha, beta);
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

	//(void)best_move;
	assert(best_index != -1);
	assert(best_index < n->child_ct);

	analyze_end:
	tree_get(gt, n);
	//if(depth)
		tree_swap_children(gt, 0, best_index);
	/*else
	{
		printf("minimax!\n");
		minimax(gt, depth);
	}*/

	//no legal moves??
	assert(n->child_ct);
	assert(n->children[0]);
	assert(n->children[0]->score == best);
	assert(is_win_score(best, depth) == best_full);

	n->score = best;	//fail soft

	return (result_t){best, best_full};
}


/*bool move_loses(void *pos, int move)
{
	if(!solver->is_legal(pos, move))
		return false;

	void *after = mem_malloc(solver->pos_size);
	void *next = mem_malloc(solver->pos_size);
	memcpy(after, pos, solver->pos_size);
	solver->make_move(after, move, NULL);

	bool losing = false;
	for(int i=0; i<solver->possible_moves; i++)
	{
		if(!solver->is_legal(after, i))
			continue;
		memcpy(next, after, solver->pos_size);
		solver->make_move(next, i, NULL);
		int endstate = solver->gameover(next);
		if(endstate == END_P1_WON || endstate == END_P2_WON)
		{
			losing = true;
			break;
		}
	}

	mem_free(after);
	mem_free(next);
	return losing;
}*/

int order_compare(const void *aa, const void *bb)
{
	const sorter_t *a = aa;
	const sorter_t *b = bb;

	return (a->score > b->score)? -1 : 1;
}

int build_movelist(sorter_t *order, void *pos)
{
	int ct = 0;
	//void *pos = node_get_pos(n);

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

//int build_order(sorter_t *order, tree_t *gt, tnode_t *n, int depth)
void sort_movelist(sorter_t *order, int len, tree_t *gt, tnode_t *n, int depth)
{
	//int ct = 0;
	void *pos = node_get_pos(n);
	//(void)pos;
	trans_value_t *v = tt_get(n, depth);
	int best = v? v->best_move : -1;

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

	for(int i=0; i<len; i++)
	{
		//get move
		int move = order[i].move;
		assert(move < solver->possible_moves);
		assert(order[i].score == 0);

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
		//order[ct].score -= (float)i/100;

		if(solver->default_order)
			order[i].score += solver->default_order[move];


		//ct++;
	}

	//must have found at least 1 legal move
	//assert(ct);

	//sort
	qsort(order, len, sizeof(*order), order_compare);

	//tree_get(gt, n);
	//tree_delete_all_but_first(gt, 0);

	//return ct;
	return;
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

/*tnode_t *create_next_order_child(tree_t *gt, tnode_t *n, int index)
{
	int m = get_next_order_move(n, index);


	tree_get(gt, n);	//do i need?

	//check if move already has a node
	tnode_t *already_made = NULL;
	for(int j=0; j<n->child_ct; j++)
	{
		if(n->children[j]->move_index == i)
		{
			already_made = n->children[j];
			break;
		}
	}
	//if(already_made)
	//	continue;

		if(solver->is_legal(n->data, i))
		{
			tree_add_copies(gt, 1);
			tnode_t *child = n->children[n->child_ct-1];
			solver->make_move(child->data, i);
			child->move_index = i;
		}
	}
}

int get_next_order_move(tnode_t *n, int index)
{
	int i = -1;
	int best_move = -1;
	trans_value_t *val = tt_get(n);
	if(val)
	{
		best_move = val->best_move;
	}

	if(index == 0 && best_move != -1)
		return best_move;
	else
		if(solver->default_order)
		{
			for(int i=0; i<solver->possible_moves; i++)
			{
				//if(solver->default_order[i] == best_move)
			}
			if(solver->default_order[index] == best_move)
				return solver->default_order[index+1];
			else
				return solver->default_order[index];
		}
		else
		{
			if(index == best_move)
				return index+1;
			else
				return index;
		}

}*/

void tt_create(void)
{

	//gdata_hash_size = sizeof(gdata_t) + solver->hash_size;

	//create the table
	trans_tbl = hashmap_create(solver->hash_size,
		sizeof(trans_value_t),
		solver->transtbl_buckets_ct);
	if(!trans_tbl)
	{
		printf("failed to allocate transposition table\n");
		exit(0);
	}
	if(solver->hash)
		hashmap_attach_hash(trans_tbl, solver->hash);
	if(solver->keys_match)
		hashmap_attach_keycompare(trans_tbl,
			solver->keys_match);
	//if(solver->normalize_position)
	//	hashmap_attach_normalize(trans_tbl, solver->normalize_position);
	if(solver->replace_transpose)
		hashmap_attach_replace(trans_tbl, solver->replace_transpose);

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

int tt_add(tnode_t *n, result_t *result, int depth, int bound, int best_move)
{
	void *pos = node_get_pos(n);
	uint32_t *hash = node_get_hash(n);

	trans_value_t value =
	{
		.score = result->score,
		.full = result->full,
		.bound = bound,
		.iddfs = iddfs_depth,
		.depth = depth,
		.best_move = best_move,
	};


	return hashmap_add_kvpair(trans_tbl, pos, &value, hash);
}

trans_value_t *tt_get(tnode_t *n, int depth)
{
	void *pos = node_get_pos(n);
	uint32_t *hash = node_get_hash(n);
	trans_value_t *value = hashmap_key_get_value(trans_tbl, pos, hash);

	if(!value && solver->flip && depth<=solver->flip_depth)
	{
		uint8_t flipped[solver->pos_size];
		solver->flip(flipped, pos);
		value = hashmap_key_get_value(trans_tbl, flipped, NULL);
	}

	//if(value)
	//	n->score = value->score;
	return value;
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
	float worst = (max_or_min(depth)==MAX_LAYER)? -WIN_SCORE : WIN_SCORE;
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

void minimax(tree_t *gt, int depth)
{
	//assert(!tree_is_leaf(gt));
	if(tree_is_leaf(gt))
	{
		//assert(0);
		return;
	}

	/*printf("doing minimax on node w %d children\n", gt->p->child_ct);
	for(int i=0; i<gt->p->child_ct; i++)
		printf("\tchild %d has score %d\n", i, gt->p->children[i]->score);
	*/

	//sort children by score (ascending or descending)
	bool order = max_or_min(depth);
	tree_attach_compare_fn(gt, (order==MAX_LAYER)?
		compare_by_score_ascending : compare_by_score_descending);
	tree_sort_children(gt);

	//set node score to child 0's score
	//tree_set_score(gt, gt->p->children[0]->score);

	//calculate "mate in n moves"?
	//if(gt->p->score >= 100)		gt->p->score++;
	//if(gt->p->score <= -100)	gt->p->score--;
}

bool max_or_min(int depth)
{
	bool mm = !(depth & 0b1);
	if(!who_goes_first)
		mm = !mm;
	return mm? MAX_LAYER : MIN_LAYER;
}

void clear_suboptimal_nodes(tree_t *gt, tnode_t *n, int depth, int var_length)
{
	tree_get(gt, n);
	if(depth >= var_length*2)
	{
		//while(n->child_ct > 0)
		//	tree_delete_child(gt, 0);
		tree_delete_all_but_first(gt, 0);
	}
	else if(depth)
	{
		//while(n->child_ct > INNER_VAR_CT)
		//	tree_delete_child(gt, INNER_VAR_CT);
		tree_delete_all_but_first(gt, INNER_VAR_CT);
	}
	else	//root node
	{
		//while(n->child_ct > PRINCIPAL_VAR_CT)
		//	tree_delete_child(gt, PRINCIPAL_VAR_CT);
		tree_delete_all_but_first(gt, PRINCIPAL_VAR_CT);
	}
}

tnode_t *node_make_new_move(tree_t *gt, tnode_t *n, int move)
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
		//*hp = *node_get_hash(n);


		solver->make_move(node_get_pos(child), move, hp);
		//solver->make_move_temp(node_get_pos(child), node_get_pos(n), move, hp);
		child->move_index = move;
		assert(child->score == 0);


	}
	return child;
}

void *node_get_pos(tnode_t *n)
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
}
