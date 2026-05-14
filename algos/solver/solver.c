/*performance metrics:
* how many positions were evaluated?
* how much time to run?
* what was the largest tree size/memory?
*/

#include "solver.h"

#include "../../ds/tree.h"
#include "../../ds/hashmap.h"
#include "../../misc/timing.h"
#include "../../memory/alloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

enum
{
	MAX_LAYER = true,
	MIN_LAYER = false
};

//statics
void tt_create(void);
int tt_add(tnode_t *n, float score, int depth, int best_move);

float eval(tree_t *gt, tnode_t *n, int depth,
	float alpha, float beta, int killer);
void build_order(int *order, tree_t *gt, tnode_t *n);
void add_all_new_moves(tree_t *gt, tnode_t *n, int depth);
void order_children(tree_t *gt, tnode_t *n, int depth, int killer);
float analyze_all_children(tree_t *gt, tnode_t *n, int *order,
	int depth, float alpha, float beta);

bool tt_check(tnode_t *n);
trans_value_t *tt_get(tnode_t *n);

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
bool max_or_min(int depth);

tnode_t *node_make_new_move(tree_t *gt, tnode_t *n, int move);
void *node_get_pos(tnode_t *n);
uint32_t *node_get_hash(tnode_t *n);

solver_t *solver;
hashmap_t *trans_tbl;
bool who_goes_first = true;
bool full_solve;
uint32_t position_ct = 0, max_node_ct = 0;
int iddfs;

typedef struct
{
	uint32_t hash;
	uint8_t pos[];
} gdata_t;
size_t gdata_size;

uint32_t passed=0, checked=0;

void print_eval_bar(float score)
{
	int len = 40;
	char *indent = "\t\t\t\t\t\t";

	printf(indent);
	for(int i=0; i<len/2-1; i++)
		printf(" ");
	//printf("%.1f\n", score);
	print_score(score);
	printf("\n");

	printf(indent);
	printf("[");
	float dist = (score+MATE_LIMIT)/(2*MATE_LIMIT)*len;
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

float test_pos(solver_t *game_solver, int *seq, int len)
{
	solver = game_solver;

	void *pos = mem_malloc(solver->pos_size);
	memcpy(pos, solver->initial_pos, solver->pos_size);

	for(int i=0; i<len; i++)
	{
		if(solver->is_legal(pos, seq[i]))
			solver->make_move(pos, seq[i], NULL);
		else
		{
			printf("illegal move %d\n", i);
			return 0;
		}
	}

	float score = solve(game_solver, pos, 10000);
	solver->draw_full(pos);
	return score;
}

float solve(solver_t *game_solver, void *pos, int time_lim_ms)
{
	solver = game_solver;

	position_ct = 0;
	max_node_ct = 0;
	full_solve = true;


	//make the tree
	gdata_size = sizeof(gdata_t) + solver->pos_size;
	//tree_t *gt = tree_create(solver->pos_size);
	tree_t *gt = tree_create(gdata_size);
	tree_clear_search_depth(gt);
	tree_attach_print_fn(gt, solver->print_pos);
	#ifdef USE_TRANSPOSITION_TABLE
	tt_create();
	#endif	//USE_TRANSPOSITION_TABLE

	if(!pos)
		pos = solver->initial_pos;
	//tree_add(gt, pos);
	gdata_t *th = mem_malloc(gdata_size);
	th->hash = solver->hash(pos, solver->pos_size);
	memcpy(&th->pos, pos, solver->pos_size);
	tree_add(gt, th);
	free(th);



	who_goes_first = solver->whosemove(pos);
	printf("player %d to move\n", who_goes_first? 1 : 2);
	//exit(0);



	if(solver->print_pos)
	{
		printf("solving game position: ");
		solver->print_pos(node_get_pos(gt->head));
	}
	else
		printf("solving...");
	printf("\n\n");

	tic();

	//int limit = 14;
	int limit = 50;
	if(solver->estimate)
		tree_set_search_depth(gt, limit);

	//dfs
	//eval(gt, gt->head, 0, -INF, INF);

	//iddfs
	for(iddfs=0; iddfs<=limit; iddfs++)
	{
		full_solve = true;

		printf("\niddfs depth=%d\n", iddfs);
		tree_set_search_depth(gt, iddfs);
		uint32_t last = toc_ms();
		eval(gt, gt->head, 0, -WIN_SCORE, WIN_SCORE, -1);
		//eval(gt, gt->head, 0, -5, 5, -1);
		uint32_t now = toc_ms();
		printf("\ttook %u ms\n", now-last);
		last = now;

		if((toc_ms() >= time_lim_ms) && !(iddfs & 0b1))
			break;

		if(full_solve)
			break;

		if(iddfs >= 1)
		{
			printf("\tpv: ");
			tnode_t *n = gt->head->children[0];
			for(int i=0; i<2*VARIATION_LENGTH; i++)
			{
				if(i >= iddfs)
					break;
				if(!n)
					break;
				printf("%d, ", n->move_index);
				if(!n->child_ct)
					break;
				//assert(n->child_ct);
				n = n->children[0];
			}
		}

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

	//output
	printf("\n\n");
	tree_draw(gt, VARIATION_LENGTH*2);

	//printf("%d out of %d passed\n", passed, checked);

	int best_move = gt->head->children[0]->move_index;
	printf("\n--- search ran to depth = %d%s ---\n", iddfs,
		full_solve? " (full solve)":"");
	printf("best move: \t\t%d\n\n\n", best_move);
	print_eval_bar(gt->head->children[0]->score);
	//printf("eval: %+.1f\n", gt->head->children[0]->score);
	printf("\nposition solved in %d m, %d sec\n", min, sec);
	printf("time per position: %.2f us\n", ((float)us)/position_ct);
	printf("evaluated %u unique positions\n", position_ct);
	printf("greatest number of nodes stored in tree: %u\n", max_node_ct);
	#ifdef USE_TRANSPOSITION_TABLE
	printf("hashmap load factor = %d%%\n", hashmap_load(trans_tbl));
	printf("number of collisions: %u\n", hashmap_collisions(trans_tbl));
	#endif


	//tree_clear(gt);
	tree_destroy(gt);
	hashmap_destroy(trans_tbl);

	return best_move;
}

//recursive
float eval(tree_t *gt, tnode_t *n, int depth,
	float alpha, float beta, int killer)
{
	if(depth == 0)
		assert(max_or_min(depth) == MAX_LAYER);
	assert(n);
	if(depth > 1000)
	{
		printf("error! depth is crazy big lol\n");
		exit(0);
	}
	void *pos = node_get_pos(n);

	#ifdef USE_TRANSPOSITION_TABLE
	//check if position was already analyzed
	//if(tt_check(n))
	//	return n->score;
	trans_value_t *val = tt_get(n);
	if(val && (val->iddfs == iddfs))
		return n->score;
	#endif

	//update metrics
	position_ct++;
	if(gt->node_ct > max_node_ct)
		max_node_ct = gt->node_ct;

	//evaluate end states/leaf nodes
	int endstate = solver->gameover(pos);
	if(endstate)
	{
		switch(endstate)
		{
			case END_P1_WON:	n->score = WIN_SCORE;	break;
			case END_P2_WON:	n->score = -WIN_SCORE;	break;
			case END_DRAW:		n->score = 0;			break;
			default:	printf("invalid endstate!\n"); exit(0);
		}
		return n->score;
	}
	else if(depth >= gt->search_depth)
	{
		full_solve = false;
		if(solver->estimate)
			n->score = solver->estimate(pos);
		else
			n->score = 0;
		return n->score;
	}

	//main analysis -- recursive tree search
	int order[solver->possible_moves];
	build_order(order, gt, n);

	/*tree_get(gt, n);
	while(n->child_ct)
		tree_delete_child(gt, 0);*/


	//add_all_new_moves(gt, n, depth);
	//order_children(gt, n, depth, killer);
	//int *order;
	float score = analyze_all_children(gt, n, order, depth, alpha, beta);

	assert(n->child_ct);
	int best_move = n->children[0]->move_index;

	#ifdef CLEAR_SUB_NODES
	//clear nodes except the best one(s)
	clear_suboptimal_nodes(gt, n, depth, VARIATION_LENGTH);
	#endif

	#ifdef USE_TRANSPOSITION_TABLE
		//hashmap_add_kvpair(trans_tbl, n->data, &n->score);
		tt_add(n, n->score, depth, best_move);
		//printf("\nadding pos: ");
		//solver->print_pos(n->data);
	#endif

	//return n->score;
	return score;
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

	for(int a=0; a<solver->possible_moves; a++)
	{
		int i;
		if(solver->default_order)
			i = solver->default_order[a];
		else
			i = a;
		//int i = order[a];
	//for(int i=0; i<solver->possible_moves; i++)
	//{
		tree_get(gt, n);	//do i need?

		//check if move already has a node
		/*tnode_t *already_made = NULL;
		for(int j=0; j<n->child_ct; j++)
		{
			if(n->children[j]->move_index == i)
			{
				already_made = n->children[j];
				break;
			}
		}
		if(already_made)
			continue;*/
		if(already_made[i])
			continue;

		/*if(solver->is_legal(node_get_pos(n), i))
		{
			tree_add_copies(gt, 1);
			tnode_t *child = n->children[n->child_ct-1];
			solver->make_move(node_get_pos(child), i);
			child->move_index = i;
		}*/
		node_make_new_move(gt, n, i);
	}

	assert(n->child_ct <= solver->possible_moves);

}

void order_children(tree_t *gt, tnode_t *n, int depth, int killer)
{
	/*printf("before sorting @ d=%d:\n", depth);
	for(int i=0; i<n->child_ct; i++)
		printf("%.2f (m%d), ", n->children[i]->score, n->children[i]->move_index);
	printf("\n");*/

	int best = -1;
	trans_value_t *val = tt_get(n);
	if(val)
		best = val->best_move;

	//assign sort scores
	tree_get(gt, n);
	//int stored = 0;
	float s_default = 0;
	for(int i=0; i<n->child_ct; i++)
	{
		//tt_get(n->children[i]);
		int end = solver->gameover(n->children[i]);

		if(end == END_P1_WON || end == END_P2_WON)
			n->children[i]->score = 1000 * (max_or_min(depth)? 1 : -1);
		else if(n->children[i]->move_index == best)
			n->children[i]->score = 999 * (max_or_min(depth)? 1 : -1);
		//else if(val)
		//	n->children[i]->score = val->score;
		//else if(n->children[i]->move_index == n->move_index)
		//		n->children[i]->score = 999 * (max_or_min(depth)? 1 : -1);
		else
			n->children[i]->score = s_default;
		s_default -= max_or_min(depth)? 0.01 : -0.01;

		/*if(val)
		{
			//n->children[i]->score = val->score;
			tree_swap_children(gt, stored, i);
			stored++;
		}
		else
			n->score = 0;
		//	assert(n->score == 0);
		*/
	}

	//


	tree_get(gt, n);
	bool order = max_or_min(depth);
	tree_attach_compare_fn(gt, (order==MAX_LAYER)?
		compare_by_score_ascending : compare_by_score_descending);
	//
	//sort the first n children (those with transtab values)
	//qsort(n->children, stored, sizeof(void*), gt->compare_fp);

	tree_sort_children(gt);

	/*printf("after sorting @ d=%d:\n", depth);
	for(int i=0; i<n->child_ct; i++)
		printf("%.2f (m%d), ", n->children[i]->score, n->children[i]->move_index);
	printf("\n");
	trans_value_t *v = tt_get(n);
	if(v)
		printf("best move stored is %d, sorted first search is %d\n",
			v->best_move, n->children[0]->move_index);
	printf("\n\n");*/
	/*
	static int total_stored = 0;
	total_stored += stored;
	if(iddfs == 10 && depth == 5)
	{
		printf("depth = %d\n", depth);
		printf("found %d of %d nodes w transtab\n", stored, n->child_ct);
		printf("total stored = %d\n", total_stored);

		for(int i=0; i<n->child_ct; i++)
			printf("score = %f, move = %d\n",
			n->children[i]->score, n->children[i]->move_index);
		printf("\n");
	}*/

	//clear sort scores
	for(int i=0; i<n->child_ct; i++)
		n->children[i]->score = 0;

	//for(int i=0; i<n->child_ct; i++)
	//	printf("%d, ", n->children[i]->move_index);
	//printf("\n");
}

float analyze_all_children(tree_t *gt, tnode_t *n, int *order,
	int depth, float alpha, float beta)
{
	/*if(depth == 0)
	{
		printf("-----------------------\n");
		printf("depth=%d\n", depth);
		printf("initial: ab=[%.1f,%.1f]\n", alpha, beta);
	}*/

	/*if(n->child_ct)
	{
		printf("\nnode supposed to have no children yet!\n");
		printf("at depth %d with %d children\n", depth, n->child_ct);
		printf("iddfs = %d\n", iddfs);
		assert(0);
	}*/

	//assert(n->child_ct == 0);

	float best = worst_score(depth);
	int best_move = -1;
	//bool cutoff = false;

	int killer = -1;

	//quick check to see if moves win
	/*for(int i=0; i<n->child_ct; i++)
	{
		tnode_t *child = n->children[i];
		int i_won = max_or_min(depth)? END_P1_WON : END_P2_WON;
		if(solver->gameover(child->data) == i_won)
		{
			child->score = max_or_min(depth)? WIN_SCORE : -WIN_SCORE;
			tree_get(gt, n);
			minimax(gt, depth);
			return child->score;
		}

	}*/

	//bool bound_changed = false;
	//for(int i=0; i<n->child_ct; i++)
	for(int i=0; i<solver->possible_moves; i++)
	{

		tree_get(gt, n);
		int move = order[i];
		if(move == -1)
			continue;
		assert(0 <= move && move < solver->possible_moves);
		tnode_t *child = node_make_new_move(gt, n, move);
		if(!child)
			continue;
		//tnode_t *child = n->children[i];

		/*float c_score;
		if(bound_changed)
		{
			if(max_or_min(depth)==MAX_LAYER)
				c_score = eval(gt, child, depth+1,
					alpha, alpha+2, killer);
			else
				c_score = eval(gt, child, depth+1,
					beta-2, beta, killer);
		}
		else
			c_score = eval(gt, child, depth+1,
			alpha, beta, killer);
		*/
		float c_score = eval(gt, child, depth+1,
			alpha, beta, killer);

		if(c_score > MATE_LIMIT)
		{
			c_score--;
			if(max_or_min(depth)==MAX_LAYER)
			{
				n->score = c_score;
				best_move = child->move_index;
				tree_get(gt, n);
				//minimax(gt, depth);
				if(n->child_ct-1)
					tree_swap_children(gt, 0, n->child_ct-1);
				return n->score;
			}
		}
		else if(c_score < -MATE_LIMIT)
		{
			c_score++;
			if(max_or_min(depth)!=MAX_LAYER)
			{
				n->score = c_score;
				best_move = child->move_index;
				tree_get(gt, n);
				//minimax(gt, depth);
				if(n->child_ct-1)
					tree_swap_children(gt, 0, n->child_ct-1);
				return n->score;
			}
		}

		/*if((max_or_min(depth)==MAX_LAYER && c_score > MATE_LIMIT)
			|| (max_or_min(depth)!=MAX_LAYER && c_score < -MATE_LIMIT))
		{
			best = c_score;
			break;
		}*/


		//if(is_better(cscore, best_so_far, depth))
		//	best_so_far = cscore;
		//(void)c_score;
		if(max_or_min(depth)==MAX_LAYER)
		{
			//best = max(best, c_score);
			if(c_score > best)
			{
				best = c_score;
				best_move = n->child_ct-1;
			}
		}
		else	//min
		{
			//best = min(best, c_score);
			if(c_score < best)
			{
				best = c_score;
				best_move = n->child_ct-1;
			}
		}

		#ifdef USE_ALPHABETA_PRUNING
		if(max_or_min(depth)==MAX_LAYER)
		{
			if(c_score > alpha)
			{
				alpha = c_score;
				//bound_changed = true;
			}
		}
		else	//min
		{
			if(c_score < beta)
			{
				beta = c_score;
				//bound_changed = true;
			}
		}
		/*	alpha = max(alpha, c_score);
		else	//min
			beta = min(beta, c_score);
		*/
		//if(depth == 1)
		//	printf("\t");
		//if(depth <= 1)
		//	printf("score is %.1f --> ab=[%.1f,%.1f]\n", c_score, alpha, beta);
		if(alpha >= beta)
		{
			//if(depth == 1)
			//	printf("\t");
			//if(depth <= 1)
			//	printf("cutoff!\n");

			if(max_or_min(depth) == MAX_LAYER)
				//&& (-MATE_LIMIT <= c_score && c_score <= MATE_LIMIT))
				{alpha++; best++;}
			else
				{beta--; best--;}
			//cutoff = true;
			//if(depth == 1)
			//	printf("\t");
			//if(depth <= 1)
			//	printf("(ab updated to [%.1f,%.1f])\n", alpha, beta);

			/*tree_get(gt, n);
			while(n->child_ct > i+1)
				tree_delete_child(gt, i+1);*/

			break;

			//n->score = beta;
			//return beta;
			//return;
		}
		#endif

		//if(i==0 && child->children[0])
		//	killer = child->children[0]->move_index;
		//printf("%d\n", killer);
	}

	assert(best_move != -1);
	tree_get(gt, n);
	minimax(gt, depth);
	//tree_swap_children(gt, 0, best_move);
	assert(n->child_ct);
	assert(n->children[0]);
	//n->score = n->children[0]->score;
	if(!(n->children[0]->score == best))
	{
		//printf("!!! minimax child score = %.2f, best is %.2f\n",
		//	n->children[0]->score, best);
		//assert(0);
	}


	/*if(max_or_min(depth)==MAX_LAYER)
	{
		n->score = best;
		//n->score = alpha;
		//if(cutoff)
		//	n->score--;
		//return alpha;
	}
	else	//min
	{
		n->score = best;
		//n->score = beta;
		//if(cutoff)
		//	n->score++;
		//return beta;
	}*/
	n->score = best;

	/*if(n->score > MATE_LIMIT)
	{
		n->score--;
		if(cutoff)
			n->score--;
	}
	else if(n->score < -MATE_LIMIT)
	{
		n->score++;
		if(cutoff)
			n->score++;
	}*/



	return n->score;
}

bool move_loses(void *pos, int move)
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
}

void build_order(int *order, tree_t *gt, tnode_t *n)
{
	int set = 0;
	int move;
	bool added[solver->possible_moves];
	for(int i=0; i<solver->possible_moves; i++)
		added[i] = false;

	trans_value_t *val = tt_get(n);
	if(val)
	{
		n->score = 0;
		move = val->best_move;

		//printf("best move is %d\n", move);
		if(!added[move])
		{
			added[move] = true;
			order[set] = move;
			set++;
		}
	}

	/*move = n->move_index;
	if(!added[move])
	{
		added[move] = true;
		order[set] = move;
		set++;
	}*/

	//add default order moves
	//int losing_ct = 0;
	//int losing[7];
	for(int i=0; i<solver->possible_moves; i++)
	{
		move = solver->default_order[i];
		if(added[move])
			continue;

		/*if(solver->move_loses(((gdata_t *)n->data)->pos, move))
		{
			losing[losing_ct] = move;
			losing_ct++;
			continue;
		}*/

		added[move] = true;
		order[set] = move;
		set++;
	}

	/*for(int i=0; i<losing_ct; i++)
	{
		order[i+set] = losing[i];
	}*/

	assert(set == solver->possible_moves);
	for(int i=0; i<solver->possible_moves; i++)
		assert(order[i] < solver->possible_moves);

	/*printf("order: ");
	for(int i=0; i<solver->possible_moves; i++)
		printf("%d, ", order[i]);
	printf("\n\n");*/

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
	//create the table
	trans_tbl = hashmap_create(gdata_size, sizeof(trans_value_t),
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

int tt_add(tnode_t *n, float score, int depth, int best_move)
{
	void *pos = node_get_pos(n);
	uint32_t *hash = node_get_hash(n);

	trans_value_t value =
	{
		.score = score,
		.iddfs = iddfs,
		.depth = depth,
		.best_move = best_move,
	};


	return hashmap_add_kvpair(trans_tbl, pos, &value, hash);
}

/*trans_value_t *tt_key_get_value(void *pos)
{
	//float *ttval = hashmap_key_get_value(trans_tbl, pos);
	trans_value_t *val = hashmap_key_get_value(trans_tbl, pos);
}*/

/*bool tt_check(tnode_t *n)
{
	void *pos = n->data;
	float *ttval = hashmap_key_get_value(trans_tbl, pos);
	if(ttval)
	{
		float prev_score = *ttval;
		n->score = prev_score;
	}
	return ttval;
}*/

trans_value_t *tt_get(tnode_t *n)
{
	void *pos = node_get_pos(n);
	uint32_t *hash = node_get_hash(n);
	trans_value_t *value = hashmap_key_get_value(trans_tbl, pos, hash);
	if(value)
		n->score = value->score;
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
		printf("%.1f", score);
	else
	{
		int dif = WIN_SCORE - abs_f(score);
		int m = dif/2;
		printf("M%d", m);
	}
}

void minimax(tree_t *gt, int depth)
{
	//assert(!tree_is_leaf(gt));
	if(tree_is_leaf(gt))
	{
		assert(0);
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
	if(depth >= var_length*2)
	{
		while(n->child_ct > 0)
			tree_delete_child(gt, 0);
	}
	else if(depth)
	{
		while(n->child_ct > INNER_VAR_CT)
			tree_delete_child(gt, INNER_VAR_CT);
	}
	else	//root node
	{
		while(n->child_ct > PRINCIPAL_VAR_CT)
			tree_delete_child(gt, PRINCIPAL_VAR_CT);
	}
}

tnode_t *node_make_new_move(tree_t *gt, tnode_t *n, int move)
{
	tnode_t *child = NULL;
	if(solver->is_legal(node_get_pos(n), move))
	{
		tree_get(gt, n);
		tree_add_copies(gt, 1);
		child = n->children[n->child_ct-1];

		uint32_t *hp = node_get_hash(child);


		solver->make_move(node_get_pos(child), move, hp);
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
