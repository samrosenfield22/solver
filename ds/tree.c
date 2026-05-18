

#include "tree.h"
#include "vector.h"
#include "../memory/alloc.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

#define USE_BLOCK_ALLOC
#define BLOCK_ALLOC_INIT_LEN	(200)

//statics
tnode_t *tree_node_create(tree_t *t, void *data);
void tree_node_add_child(tnode_t *p, tnode_t *n);
void tree_node_print(tnode_t *n);
//void *tree_next_dfs(tree_t *t, int *depth);
//void *tree_next_bfs(tree_t *t, int *depth);
//void *tree_next_iddfs(tree_t *t, int *depth);
bool searchlist_does_node_get_removed(tree_t *t);
void searchlist_add_children(tree_t *t, tnode_t *n, int cdepth);
void searchlist_add_node(tree_t *t, tnode_t *n, int cdepth);
void node_delete(tree_t *t, tnode_t *n);

tnode_t *node_and_data_alloc(tree_t *t);
void node_and_data_free(tree_t *t, tnode_t *n);

tree_t *tree_create(size_t size)
{
	tree_t *t = mem_malloc(sizeof(*t));
	if(!t)
		return NULL;

	t->item_size = size;
	t->node_ct = 0;
	t->p = NULL;
	t->head = NULL;
	t->search_list = NULL;
	t->traverse_mode = TRAVERSE_DFS;
	t->print_fp = NULL;
	t->destroy_fp = NULL;
	t->compare_fp = compare_by_score_ascending;
	t->depth = 0;
	t->first_visit = false;

	t->search_depth = NO_DEPTH_LIMIT;

	#ifdef USE_BLOCK_ALLOC
	t->node_allocator = block_allocator(sizeof(tnode_t), BLOCK_ALLOC_INIT_LEN);
	t->data_allocator = block_allocator(size, BLOCK_ALLOC_INIT_LEN);
	#endif

	return t;
}

typedef struct
{
	tnode_t *n;
	int depth;
	bool visited;
} node_with_meta_t;

void *tree_first(tree_t *t, int *depth)
{
	//create new helper stack/queue
	if(t->search_list)
		list_destroy(t->search_list);
	t->search_list = list(node_with_meta_t);

	//push the head
	tree_get_head(t);
	searchlist_add_node(t, t->head, 0);
	//node_with_meta_t nwm = {.n=t->head, .depth=0, .visited=false};
	//list_push(t->search_list, &nwm);

	return tree_next(t, depth);
}

//
void *tree_next(tree_t *t, int *depth)
{
	if(list_isempty(t->search_list))
	{
		t->p = NULL;
		return NULL;
	}

	//pop/deque
	node_with_meta_t *meta;
	if(searchlist_does_node_get_removed(t))
	{
		if(t->traverse_mode == TRAVERSE_BFS)
			meta = list_deq(t->search_list);
		else
			meta = list_pop(t->search_list);
	}
	else
		meta = list_peek(t->search_list);


	//tnode_t *n = meta->n;
	tree_get(t, meta->n);
	t->depth = meta->depth;
	t->first_visit = !meta->visited;
	//if(t->first_visit == false)
	//	exit(0);
	if(depth)
		*depth = meta->depth;

	//add the node's children to the searchlist
	if(!meta->visited)
		searchlist_add_children(t, meta->n, meta->depth+1);

	meta->visited = true;
	return meta->n->data;
}


bool tree_islast(tree_t *t)
{
	//if(t->p == NULL)
	//	printf("found last!\n");
	return (t->p == NULL);

	/*
	automatically destroy the search list?
	bool islast = (t->p == NULL);
	if(islast)
	{
		if(t->search_list)
		{
			list_destroy(t->search_list);
			t->search_list = NULL;
		}
	}
	return islast;
	*/
}

bool tree_is_leaf(tree_t *t)
{
	if(t->p == NULL)
		printf("reeeeee\n");
	return (t->p->child_ct == 0);
}

void tree_set_traverse_mode(tree_t *t, tree_traverse_modes mode)
{
	if(mode >= INVALID_TRAVERSE_MODES)
	{
		printf("[error: invalid tree traverse mode %d]\n", mode);
		exit(0);
	}
	t->traverse_mode = mode;
}

void *tree_get_head(tree_t *t)
{
	if(!t) return NULL;
	t->depth = 0;
	return tree_get(t, t->head)->data;
}

void *tree_get_child(tree_t *t, int n)
{
	if(!t) return NULL;
	t->depth++;
	return tree_get(t, t->p->children[n])->data;

	/*tnode_t *child = t->p->children[n];
	t->p = child;
	return child->data;*/
}


void tree_add(tree_t *t, void *data)
{
	if(!t) return;

	tnode_t *n = tree_node_create(t, data);
	if(!t->head)
	{
		t->head = n;
		t->p = n;
	}
	else
	{
		tree_node_add_child(t->p, n);
		if(t->search_list)
		{
			//node_with_meta_t *pwm = list_last(t->search_list);
			//assert(pwm->n == t->p);
			searchlist_add_node(t, n, t->depth+1);
		}
	}
}

void tree_add_copies(tree_t *t, int cnt)
{
	if(!t) return;

	for(int i=0; i<cnt; i++)
	{
		//printf("adding copy %d\n", i);
		tree_add(t, t->p->data);
	}
}

void tree_add_empties(tree_t *t, int cnt)
{
	if(!t) return;

	for(int i=0; i<cnt; i++)
	{
		tree_add(t, NULL);
	}
}

void tree_destroy(tree_t *t)
{
	if(!t)	return;

	tree_clear(t);
	list_destroy(t->search_list);

	#ifdef USE_BLOCK_ALLOC
	block_allocator_destroy(t->node_allocator);
	block_allocator_destroy(t->data_allocator);
	#endif

	mem_free(t);
}

void tree_clear(tree_t *t)
{
	if(!t)	return;

	node_delete(t, t->head);
	if(t->node_ct)
	{
		printf("node ct not zero after clearing! %d nodes\n", t->node_ct);
		exit(0);
	}
}

void tree_delete_child(tree_t *t, int n)
{
	if(!t) 					return;
	if(!t->p)				return;
	if(n >= t->p->child_ct)	return;

	/*
	for(int i=0; i<t->p->child_ct; i++)
	{
		node_with_meta_t *meta = list_pop(t->search_list);
		if(meta->n != t->p->children[i])
		{
			printf("ruh roh!\n");
			exit(0);
		}
	}*/

	node_delete(t, t->p->children[n]);

	int nodes_after = (t->p->child_ct - n) - 1;
	memcpy(&t->p->children[n], &t->p->children[n+1],
		nodes_after * sizeof(tnode_t *));
	t->p->child_ct--;
	vector_resize(&t->p->children, t->p->child_ct);
}


const int indent = 20;
int cursor = 0;

void draw_add_space(int depth)
{
	while(cursor < (depth*indent)-1)
	{
		if((cursor % indent) == indent-2)
			putchar('|');
		else
			putchar(' ');
		cursor++;
	}
}

void tree_draw(tree_t *t, int maxdepth)
{
	tree_traverse_modes saved_mode = t->traverse_mode;
	t->traverse_mode = TRAVERSE_DFS;

	if(!t->head)
		printf("(empty)\n");
	else
	{
		int last_depth = -1;
		//int cursor = 0;
		//int indent = 8;
		cursor = 0;

		tree_iterate(t, item, depth)
		{
			if(depth > maxdepth)
				continue;

			if(depth > last_depth)
			{
				while(cursor < depth*indent)
				{
					putchar('-');
					cursor++;
				}
			}
			else if(depth == last_depth)
			{
				printf("\n");
				cursor = 0;
				draw_add_space(depth);
				//cursor += printf("| ");
				printf("\n");
				cursor = 0;
				draw_add_space(depth);
				cursor += printf("-");
			}
			else	//depth < last_depth
			{
				printf("\n");
				cursor = 0;
				draw_add_space(depth);
				printf("\n");
				cursor = 0;
				draw_add_space(depth);
				cursor += printf("-");
			}

			//cursor += printf("[%d]", *(int*)item);
			if(t->print_fp)
			{
				cursor += printf("[");
				cursor += t->print_fp(item);
				cursor += printf(" (%+.1f)]", t->p->score);
			}
			else
				cursor += printf("[%d??? (%+.1f)]", t->p->move_index, t->p->score);

			last_depth = depth;

		}
	}
	printf("\n");

	//restore
	t->traverse_mode = saved_mode;
}

void tree_set_search_depth(tree_t *t, int d)
{
	if(!t)	return;
	t->search_depth = d;
}

void tree_clear_search_depth(tree_t *t)
{
	if(!t)	return;
	t->search_depth = NO_DEPTH_LIMIT;
}

void tree_set_score(tree_t *t, float score)
{
	if(!t)
		return;

	if(t->p)
		t->p->score = score;
}

float tree_get_score(tree_t *t)
{
	if(!t)
		return 0;

	if(!t->p)
		return 0;

	return t->p->score;
}

void tree_sort_children(tree_t *t)
{
	if(!t->compare_fp)
		return;

	//qsort(t->p->children, t->p->child_ct, t->item_size, t->compare_fp);
	qsort(t->p->children, t->p->child_ct, sizeof(void*), t->compare_fp);
}

void tree_swap_children(tree_t *t, int a, int b)
{
	if(!t)
		return;

	if(a >= t->p->child_ct || b >= t->p->child_ct)
	{
		assert(0);
		return;
	}

	tnode_t *temp = t->p->children[a];
	t->p->children[a] = t->p->children[b];
	t->p->children[b] = temp;
}

void tree_attach_print_fn(tree_t *t, int (*print_fp)(void *d))
{
	if(!t)
		return;

	t->print_fp = print_fp;
}

void tree_attach_destroy_fn(
	tree_t *t, void (*destroy_fp)(void *d))
{
	if(!t)
		return;

	t->destroy_fp = destroy_fp;
}

void tree_attach_compare_fn(
	tree_t *t, int (*compare_fp)(const void *a, const void *b))
{
	if(!t)
		return;

	t->compare_fp = compare_fp;
}





void tree_demo(void)
{
	/*tree_t *t = tree(int);

	tree_add(t, &(int){1});		//head


	tree_get_head(t);
	for(int i=0; i<5; i++)
	{
		int h = *(int*)t->p->data;
		tree_add(t, &(int){h*10+i});
	}

	tree_iterate(t, item, id)
	{
		if(id == 0)
			continue;

		for(int i=0; i<5; i++)
		{
			int h = *(int*)t->p->data;
			tree_add(t, &(int){h*10+i});
		}
	}

	tree_draw(t);

	return;
	*/

	/////////////////////////



	tree_t *mytree = tree(char *);

	tree_add(mytree, &(char*){"zero"});		//head
	tree_add(mytree, &(char*){"one"});	//c0
	tree_add(mytree, &(char*){"two"});	//c1

	tree_get_child(mytree, 0);	//c0
	tree_add(mytree, &(char*){"three"});
	tree_add(mytree, &(char*){"four"});

	tree_get_head(mytree);
	tree_get_child(mytree, 1);	//c1
	tree_add(mytree, &(char*){"5"});
	tree_add(mytree, &(char*){"6"});
	tree_add(mytree, &(char*){"7"});

	tree_get_child(mytree, 0);	//c1c0, piggy
	tree_add(mytree, &(char*){"8"});

	tree_iterate(mytree, item, d)
	{
		if(strcmp(*(char**)item, "6")==0 && d==2)
		{
			tree_add(mytree, &(char*){"6a"});
			tree_add(mytree, &(char*){"6b"});
			tree_add(mytree, &(char*){"67"});
			//tree_add_copies(mytree, 3);
		}
	}
	tree_iterate(mytree, item, d2)
	{
		if(strcmp(*(char**)item, "6b")==0)
		{
			tree_add(mytree, &(char*){"6bdeez"});
			tree_add(mytree, &(char*){"6bb"});
			tree_add(mytree, &(char*){"6blmao"});
			tree_set_score(mytree, 5);
		}
	}
	tree_iterate(mytree, item, d3)
	{
		if(strcmp(*(char**)item, "6a")==0)
		{
			tree_set_score(mytree, -3);
		}
	}
	tree_iterate(mytree, item, ddd)
	{
		if(strcmp(*(char**)item, "6")==0)
		{
			//tree_delete_child(mytree, 1);
			//printf("node has %d children\n", mytree->p->child_ct);
			//tree_sort_children(mytree);
		}
	}

	/*tree_set_traverse_mode(mytree, TRAVERSE_IDDFS);
	tree_iterate(mytree, item, dd)
	{
		if(dd == 0)
			printf("\n");
		printf("%s, ", *(char**)item);
	}*/

	printf("\n\n");
	tree_draw(mytree, NO_DEPTH_LIMIT);


	//tree_set_search_depth(mytree, 2);
	//tree_set_traverse_mode(mytree, TRAVERSE_BFS);
	//tree_set_traverse_mode(mytree, TRAVERSE_DFS);
	tree_set_traverse_mode(mytree, TRAVERSE_DFS_EULERTOUR);
	printf("\n");
	for(int i=0; i<5; i++)
	{
		printf("\n\n------------------ searching to depth %d --------------\n", i);
		tree_set_search_depth(mytree, i);
		tree_iterate(mytree, item, dd)
		{

			printf("%s (%s), ", *(char**)item,
				mytree->first_visit? "first":"second");
		}
	}

	tree_set_traverse_mode(mytree, TRAVERSE_DFS);
	printf("\n\n\ntraversing with dfs:\n");
	tree_clear_search_depth(mytree);
	tree_iterate(mytree, item, dd)
	{
		if(!(dd == mytree->depth))
		{
			printf("depth mismatch!\n");
			printf("%s\n", *(char**)item);
			printf("iter d=%d, tree d=%d\n", dd, mytree->depth);
			exit(0);
		}

		printf("%s, ", *(char**)item);
		if(strcmp(*(char**)item, "8")==0)
		{
			tree_add(mytree, &(char*){"ayylmao"});
			tree_get_child(mytree, 0);
			tree_add(mytree, &(char*){"ayy"});
			tree_add(mytree, &(char*){"lmao"});
		}
	}


	tree_destroy(mytree);
}

///////////////////////// statics //////////////////////////////

tnode_t *tree_node_create(tree_t *t, void *data)
{
	if(!t)
		return NULL;

	tnode_t *n = node_and_data_alloc(t);
	if(!n)
		return NULL;

	//instantiate data
	//printf("\tcopying data \'%c\' (%lu bytes)\n", *(char*)data, t->item_size);
	n->child_ct = 0;
	if(data)
	{
		/*n->data = mem_malloc(t->item_size);
		if(!n->data)
		{
			mem_free(n);
			return NULL;
		}*/
		memcpy(n->data, data, t->item_size);
	}

	//for now, each node will get a default 4 children
	//n->children = mem_malloc(4 * sizeof(*n->children));
	n->children = vector(tnode_t *, 4);
	if(!n->children)
	{
		node_and_data_free(t, n);
		return NULL;
	}


	t->node_ct++;
	n->score = 0;
	n->move_index = 0;

	return n;
}

void tree_node_print(tnode_t *n)
{
	printf("[node \'%s\' with %d children]",
	*(char**)n->data, n->child_ct);
}

void tree_node_add_child(tnode_t *p, tnode_t *n)
{
	int new_len = p->child_ct+1;
	vector_resize(&p->children, new_len);
	p->children[p->child_ct] = n;
	p->child_ct = new_len;
}

tnode_t *tree_get(tree_t *t, tnode_t *n)
{
	if(!t) return NULL;
	assert(t);

	t->p = n;
	return n;
}

//pop node, push its children in reverse order
/*void *tree_next_dfs(tree_t *t, int *depth)
{
	//pop
	node_with_meta_t *parent_meta =
		(node_with_meta_t *)list_pop(t->search_list);
	tnode_t *n = parent_meta->n;
	//printf("(d=%d)", parent_meta->depth);
	if(depth)
		*depth = parent_meta->depth;
	int cdepth = parent_meta->depth + 1;
	//tnode_t *n = *(tnode_t **)list_pop(t->search_list);
	tree_get(t, n);

	//push its children
	for(int i=n->child_ct-1; i>=0; i--)
	{
		node_with_meta_t nwm = {.n=n->children[i], .depth=cdepth, .visited=false};
		list_push(t->search_list, &nwm);
		//list_push(t->search_list, &n->children[i]);
	}

	return n->data;
}*/

//deque node, enque its children in forward order
/*void *tree_next_bfs(tree_t *t, int *depth)
{
	//deque
	node_with_meta_t *parent_meta =
		(node_with_meta_t *)list_deq(t->search_list);
	tnode_t *n = parent_meta->n;
	//printf("(d=%d)", parent_meta->depth);
	if(depth)
		*depth = parent_meta->depth;
	int cdepth = parent_meta->depth + 1;
	tree_get(t, n);
	//tnode_t *n = *(tnode_t **)list_deq(t->search_list);
	//tree_get(t, n);

	//enque its children
	for(int i=0; i<n->child_ct; i++)
	{
		node_with_meta_t nwm = {.n=n->children[i], .depth=cdepth, .visited=false};
		list_enq(t->search_list, &nwm);
		//list_enq(t->search_list, &n->children[i]);
	}

	return n->data;
}*/





/*if we're doing euler tour order, we peek at the upcoming node.
if it's our first time seeing the node, we leave it on the stack
(unless it's a leaf node) so that we'll come back to it*/
bool searchlist_does_node_get_removed(tree_t *t)
{
	bool remove_node = true;

	node_with_meta_t *meta = (node_with_meta_t *)list_peek(t->search_list);
	tree_get(t, meta->n);

	if(t->traverse_mode == TRAVERSE_DFS_EULERTOUR)
	{
		//if(!meta->visited
		//	&& !(tree_is_leaf(t) || meta->depth == t->search_depth))
		if(!meta->visited
			&& !(meta->depth == t->search_depth))
			remove_node = false;
	}

	return remove_node;
}

void searchlist_add_children(tree_t *t, tnode_t *n, int cdepth)
{
	//don't add children to be iterated if they're past
	//the search depth
	if(cdepth > t->search_depth)
		return;

	//add all children
	if(t->traverse_mode == TRAVERSE_BFS)
	{
		//enque children in forward order
		for(int i=0; i<n->child_ct; i++)
		{
			searchlist_add_node(t, n->children[i], cdepth);
			//node_with_meta_t nwm = {.n=n->children[i], .depth=cdepth, .visited=false};
			//list_enq(t->search_list, &nwm);
		}
	}
	else
	{
		//push children, in reverse order
		for(int i=n->child_ct-1; i>=0; i--)
		{
			searchlist_add_node(t, n->children[i], cdepth);
			//node_with_meta_t nwm = {.n=n->children[i], .depth=cdepth, .visited=false};
			//list_push(t->search_list, &nwm);
		}
	}
}

void searchlist_add_node(tree_t *t, tnode_t *n, int cdepth)
{
	//tnode_t *n_copy = mem_malloc(sizeof(*n_copy));
	//memcpy(n_copy, n, sizeof(*n_copy));
	//node_with_meta_t nwm = {.n=n_copy, .depth=cdepth, .visited=false};
	node_with_meta_t nwm = {.n=n, .depth=cdepth, .visited=false};

	if(t->traverse_mode == TRAVERSE_BFS)
		list_enq(t->search_list, &nwm);
	else
		list_push(t->search_list, &nwm);
}



void node_delete(tree_t *t, tnode_t *n)
{
	//delete children
	for(int i=0; i<n->child_ct; i++)
		node_delete(t, n->children[i]);

	//delete node members
	vector_destroy(n->children);
	if(t->destroy_fp)
		t->destroy_fp(n->data);

	node_and_data_free(t, n);

	t->node_ct--;
}

tnode_t *node_and_data_alloc(tree_t *t)
{
	#ifdef USE_BLOCK_ALLOC
	tnode_t *n = block(t->node_allocator);
	n->data = block(t->data_allocator);
	#else
	tnode_t *n = mem_malloc(sizeof(*n));
	n->data = mem_malloc(t->item_size);
	#endif

	return n;
}


void node_and_data_free(tree_t *t, tnode_t *n)
{
	#ifdef USE_BLOCK_ALLOC
	if(n->data)
		block_free(t->data_allocator, n->data);
	block_free(t->node_allocator, n);
	#else
	if(n->data)
		mem_free(n->data);
	mem_free(n);
	#endif
}


int compare_by_score_ascending(const void *a, const void *b)
{
	const tnode_t *an = *(void**)a, *bn = *(void**)b;

	if(bn->score > an->score)
	 	return 1;
	else if(bn->score < an->score)
		return -1;
	else
		return 0;

	//return (bn->score - an->score);
}

int compare_by_score_descending(const void *a, const void *b)
{
	//const tnode_t *an = *(void**)a, *bn = *(void**)b;
	//return (an->score - bn->score);
	return compare_by_score_ascending(b, a);
}
