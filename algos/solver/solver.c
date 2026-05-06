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
float eval(tree_t *gt, tnode_t *n, int depth, float alpha, float beta);
void add_all_new_moves(tree_t *gt, tnode_t *n);
float analyze_all_children(tree_t *gt, tnode_t *n, int depth, float alpha, float beta);

bool transposition_check(tnode_t *n);
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

typedef struct
{
	uint8_t move_index;
	void *pos;
} pos_meta;

float solve(solver_t *game_solver, void *pos)
{
	solver = game_solver;

	position_ct = 0;
	max_node_ct = 0;


	//make the tree
	tree_t *gt = tree_create(solver->pos_size);
	tree_clear_search_depth(gt);
	tree_attach_print_fn(gt, solver->print_pos);
	if(!pos)
		pos = solver->initial_pos;
	//if(pos)
		tree_add(gt, pos);
	//else
	//	tree_add(gt, solver->initial_pos);
	who_goes_first = solver->whosemove(pos);
	printf("player %d to move\n", who_goes_first? 1 : 2);
	//exit(0);

	#ifdef USE_TRANSPOSITION_TABLE
	trans_tbl = hashmap_create(solver->pos_size, sizeof(float),
		solver->transtbl_buckets_ct);
	if(!trans_tbl)
	{
		printf("failed to allocate transposition table\n");
		return 0;
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
	#endif	//USE_TRANSPOSITION_TABLE

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

	int limit = 16;
	if(solver->estimate)
		tree_set_search_depth(gt, limit);

	//dfs
	//eval(gt, gt->head, 0, -INF, INF);

	//iddfs
	for(int i=0; i<=limit; i++)
	{
		tree_set_search_depth(gt, i);
		eval(gt, gt->head, 0, -INF, INF);
		//eval(gt, gt->head, 0, -5, 5);

		printf("iddfs depth=%d\n", i);
		//tree_draw(gt, 2);
		//printf("continue:  ");
		//getchar();
		//printf("\n\n");

		if(i < limit-1)
			hashmap_clear(trans_tbl);
	}

	//count time
	uint32_t us = toc();
	uint32_t sec_total = us/1000000;
	uint32_t min = sec_total/60;
	uint32_t sec = sec_total - (60*min);

	//output
	tree_draw(gt, 2);

	int best_move = gt->head->children[0]->move_index;
	printf("\nbest move: %d\n", best_move);
	printf("eval: %+.1f\n", gt->head->children[0]->score);
	printf("\nposition solved in %d m, %d sec\n", min, sec);
	printf("time per position: %.1f us\n", ((float)us)/position_ct);
	printf("evaluated %u unique positions\n", position_ct);
	printf("greatest number of nodes stored in tree: %u\n", max_node_ct);
	printf("hashmap load factor = %d%%\n", hashmap_load(trans_tbl));


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
	if(transposition_check(n))
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



	/*if(solver->is_end(pos) || depth >= gt->search_depth)
	{
		n->score = solver->evaluate_leaf(pos);
		return n->score;
	}*/


	add_all_new_moves(gt, n);
	float score = analyze_all_children(gt, n, depth, alpha, beta);

	/*

	float best_so_far = worst_score(depth+1);



	for(int i=0; i<solver->possible_moves; i++)
	{
		tree_get(gt, n);

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

		tnode_t *child = NULL;

		//check if there's already a node with this move
		//if(solver->get_move(pos) == solver->get_move(n->data))
		//	cscore = eval(gt, n->children[0], depth+1, best_so_far);
		if(already_made)
			child = already_made;
		else if(solver->is_legal(pos, i))
		{
			tree_add_copies(gt, 1);
			child = n->children[n->child_ct-1];
			solver->make_move(child->data, i);
			child->move_index = i;
		}

		if(child)
		{
			//recurse
			float cscore = eval(gt, child, depth+1, best_so_far);
			(void)cscore;

			#ifdef USE_ALPHABETA_PRUNING
			//if(depth > 2)
			//	cscore = worst_score(depth);
			//printf("prune\n");
			if(alphabeta_cutoff(cscore, prune, &best_so_far, depth+1))
				break;
			#endif
		}
	}*/

	//n->score = score;
	//tree_get(gt, n);
	//minimax(gt, depth);
	//n->score = n->children[0]->score;
	//n->score = score;

	#ifdef CLEAR_SUB_NODES
	//clear nodes except the best one(s)
	clear_suboptimal_nodes(gt, n, depth, VARIATION_LENGTH);
	#endif

	#ifdef USE_TRANSPOSITION_TABLE
		hashmap_add_kvpair(trans_tbl, n->data, &n->score);
	#endif

	//return n->score;
	return score;
}

void add_all_new_moves(tree_t *gt, tnode_t *n)
{
	void *pos = n->data;

	//if(solver->default_order)
	//else
	//	int order[solver->possible_moves];

	//tree_get(gt, n);
	//tree_attach_compare_fn(gt, compare_by_score_descending);
	//tree_sort_children(gt);

	//int order[] = {3, 2, 4, 1, 5, 0, 6};
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

		if(solver->is_legal(pos, i))
		{
			tree_add_copies(gt, 1);
			tnode_t *child = n->children[n->child_ct-1];
			solver->make_move(child->data, i);
			child->move_index = i;
		}
	}

	//or... sort by estimate
	/*for(int i=0; i<n->child_ct; i++)
		n->score = solver->estimate(n->children[i]);
	tree_attach_compare_fn(gt, compare_by_score_descending);
	tree_sort_children(gt);
	if(n->children[0]->score)
	{
		for(int i=0; i<n->child_ct; i++)
			printf("score = %f\n", n->children[i]->score);
		printf("\n");
	}*/
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

bool transposition_check(tnode_t *n)
{
	//printf("checking trans tbl\n");
	void *pos = n->data;
	float *ttval = hashmap_key_get_value(trans_tbl, pos);
	if(ttval)
	{
		float prev_score = *ttval;
		n->score = prev_score;
	}
	return ttval;
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
