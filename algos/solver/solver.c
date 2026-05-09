/*performance metrics:
* how many positions were evaluated?
* how much time to run?
* what was the largest tree size/memory?
*/

#include "solver.h"

#include "../../ds/tree.h"
#include "../../ds/hashmap.h"
#include "../../misc/timing.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

enum
{
	MAX_LAYER = true,
	MIN_LAYER = false
};

//statics
void tt_create(void);
int tt_add(tnode_t *n, float score, int depth, int best_move);

float eval(tree_t *gt, tnode_t *n, int depth, float alpha, float beta);
void add_all_new_moves(tree_t *gt, tnode_t *n, int depth);
void order_children(tree_t *gt, tnode_t *n, int depth);
float analyze_all_children(tree_t *gt, tnode_t *n, int depth, float alpha, float beta);

bool tt_check(tnode_t *n);
trans_value_t *tt_get(tnode_t *n);

bool alphabeta_cutoff(float cscore, float prune,
	float *best_so_far, int depth);
//void evaluate(tree_t *gt, solver_t *solver);
float max(float x, float y);
float min(float x, float y);
void minimax(tree_t *gt, int depth);
void clear_suboptimal_nodes(tree_t *gt, tnode_t *n, int depth, int var_length);

bool is_better(float s0, float s1, int depth);
bool is_worse(float s0, float s1, int depth);
float worst_score(int depth);
bool max_or_min(int depth);

solver_t *solver;
hashmap_t *trans_tbl;
bool who_goes_first = true;
uint32_t position_ct = 0, max_node_ct = 0;
int iddfs;

uint32_t passed=0, checked=0;

float solve(solver_t *game_solver, void *pos)
{
	solver = game_solver;

	position_ct = 0;
	max_node_ct = 0;


	//make the tree
	tree_t *gt = tree_create(solver->pos_size);
	tree_clear_search_depth(gt);
	tree_attach_print_fn(gt, solver->print_pos);
	#ifdef USE_TRANSPOSITION_TABLE
	tt_create();
	#endif	//USE_TRANSPOSITION_TABLE

	if(!pos)
		pos = solver->initial_pos;
	//if(pos)
		tree_add(gt, pos);
	//else
	//	tree_add(gt, solver->initial_pos);
	who_goes_first = solver->whosemove(pos);
	printf("player %d to move\n", who_goes_first? 1 : 2);
	//exit(0);



	if(solver->print_pos)
	{
		printf("solving game position: ");
		solver->print_pos(gt->head->data);
	}
	else
		printf("solving...");
	printf("\n\n");

	tic();

	//evaluate all nodes
	//tree_set_traverse_mode(gt, TRAVERSE_DFS_EULERTOUR);
	/*tree_iterate(gt, item, depth)
	{
		printf("------------------------------\nchecking item\n");
		(void)item;

		evaluate(gt, solver);
	}*/

	int limit = 14;
	if(solver->estimate)
		tree_set_search_depth(gt, limit);

	//dfs
	//eval(gt, gt->head, 0, -INF, INF);

	//iddfs
	for(iddfs=0; iddfs<=limit; iddfs++)
	{
		tree_set_search_depth(gt, iddfs);
		eval(gt, gt->head, 0, -INF, INF);
		//eval(gt, gt->head, 0, -5, 5);

		printf("iddfs depth=%d\n", iddfs);
		//tree_draw(gt, 2);
		//printf("continue:  ");
		//getchar();
		//printf("\n\n");

		//if(iddfs < limit-1)
		//	hashmap_clear(trans_tbl);
	}

	//count time
	uint32_t us = toc();
	uint32_t sec_total = us/1000000;
	uint32_t min = sec_total/60;
	uint32_t sec = sec_total - (60*min);

	//output
	tree_draw(gt, 4);

	printf("%d out of %d passed\n", passed, checked);

	int best_move = gt->head->children[0]->move_index;
	printf("\nbest move: %d\n", best_move);
	printf("eval: %+.1f\n", gt->head->children[0]->score);
	printf("\nposition solved in %d m, %d sec\n", min, sec);
	printf("time per position: %.1f us\n", ((float)us)/position_ct);
	printf("evaluated %u unique positions\n", position_ct);
	printf("greatest number of nodes stored in tree: %u\n", max_node_ct);
	printf("hashmap load factor = %d%%\n", hashmap_load(trans_tbl));
	printf("number of collisions: %u\n", hashmap_collisions(trans_tbl));


	//tree_clear(gt);
	tree_destroy(gt);
	hashmap_destroy(trans_tbl);

	return best_move;
}

//recursive
float eval(tree_t *gt, tnode_t *n, int depth, float alpha, float beta)
{
	if(depth == 0)
		assert(max_or_min(depth) == MAX_LAYER);
	assert(n);
	if(depth > 1000)
	{
		printf("error! depth is crazy big lol\n");
		exit(0);
	}
	void *pos = n->data;

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
		if(solver->estimate)
			n->score = solver->estimate(pos);
		else
			n->score = 0;
		return n->score;
	}

	//main analysis -- recursive tree search
	add_all_new_moves(gt, n, depth);
	order_children(gt, n, depth);
	float score = analyze_all_children(gt, n, depth, alpha, beta);

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
		tnode_t *already_made = NULL;
		for(int j=0; j<n->child_ct; j++)
		{
			if(n->children[j]->move_index == i)
			{
				already_made = n->children[j];
				break;
			}
		}
		if(already_made)
			continue;

		if(solver->is_legal(n->data, i))
		{
			tree_add_copies(gt, 1);
			tnode_t *child = n->children[n->child_ct-1];
			solver->make_move(child->data, i);
			child->move_index = i;
		}
	}

}

void order_children(tree_t *gt, tnode_t *n, int depth)
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
		trans_value_t *val = tt_get(n->children[i]);
		if(n->children[i]->move_index == best)
			n->children[i]->score = 1000 * (max_or_min(depth)? 1 : -1);
		//else if(val)
		//	n->children[i]->score = val->score;
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
}

float analyze_all_children(tree_t *gt, tnode_t *n, int depth, float alpha, float beta)
{
	/*if(depth == 0)
	{
		printf("-----------------------\n");
		printf("depth=%d\n", depth);
		printf("initial: ab=[%.1f,%.1f]\n", alpha, beta);
	}*/

	float best = worst_score(depth);



	for(int i=0; i<n->child_ct; i++)
	{

		tree_get(gt, n);

		//recurse
		/*if(depth == 0)
		{
			printf("child %d:\n", n->children[i]->move_index);
		}*/

		tnode_t *child = n->children[i];
		float c_score = eval(gt, child, depth+1, alpha, beta);
		//if(is_better(cscore, best_so_far, depth))
		//	best_so_far = cscore;
		//(void)c_score;
		if(max_or_min(depth)==MAX_LAYER)
			best = max(best, c_score);
		else	//min
			best = min(best, c_score);


		#ifdef USE_ALPHABETA_PRUNING
		if(max_or_min(depth)==MAX_LAYER)
			alpha = max(alpha, c_score);
		else	//min
			beta = min(beta, c_score);
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
				alpha++;
			else
				beta--;

			//if(depth == 1)
			//	printf("\t");
			//if(depth <= 1)
			//	printf("(ab updated to [%.1f,%.1f])\n", alpha, beta);

			tree_get(gt, n);
			while(n->child_ct > i+1)
				tree_delete_child(gt, i+1);
			break;
			//n->score = beta;
			//return beta;
			//return;
		}
		#endif
	}

	tree_get(gt, n);
	minimax(gt, depth);
	//n->score = n->children[0]->score;
	if(!(n->children[0]->score == best))
	{
		//printf("!!! minimax child score = %.2f, best is %.2f\n",
		//	n->children[0]->score, best);
		//assert(0);
	}


	if(max_or_min(depth)==MAX_LAYER)
	{
		//n->score = best;
		n->score = alpha;
		return alpha;
	}
	else	//min
	{
		//n->score = best;
		n->score = beta;
		return beta;
	}
}

void tt_create(void)
{
	//create the table
	trans_tbl = hashmap_create(solver->pos_size, sizeof(trans_value_t),
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
	if(solver->normalize_position)
		hashmap_attach_normalize(trans_tbl, solver->normalize_position);
	if(solver->replace_transpose)
		hashmap_attach_replace(trans_tbl, solver->replace_transpose);

	//test the hash table functionality with the current solver
	tree_t *test = tree_create(solver->pos_size);
	tree_add(test, solver->initial_pos);
	for(int i=0; i<4; i++)
	{
		int move = rand() % solver->possible_moves;
		if(solver->is_legal(test->p->data, move))
			solver->make_move(test->p->data, move);
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
	void *pos = n->data;

	trans_value_t value =
	{
		.score = score,
		.iddfs = iddfs,
		.depth = depth,
		.best_move = best_move,
	};
	return hashmap_add_kvpair(trans_tbl, pos, &value);
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
	void *pos = n->data;
	trans_value_t *value = hashmap_key_get_value(trans_tbl, pos);
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
	float worst = (max_or_min(depth)==MAX_LAYER)? -INF : INF;
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
	else
	{
		while(n->child_ct > PRINCIPAL_VAR_CT)
			tree_delete_child(gt, PRINCIPAL_VAR_CT);
	}
}

/*evaluate(node n, depth d):

for each node (iterate euler dfs to depth d)
	if first visit
		build game position
		if depth == d
			evaluate leaf node
		else
			for each possible move
				copy child (give it its move index)
	else if second visit
		minimax

minimax(node, depth)
	sort children by score (ascending or descending)
	set node score to child 0's score
	delete all other children (try this later)
*/

void solver_demo(void)
{
	solver_t thegame_solver =
	{

	};

	solve(&thegame_solver, NULL);	//from initial position
}
