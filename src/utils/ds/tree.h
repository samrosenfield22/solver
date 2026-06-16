

#ifndef TREE_H_
#define TREE_H_

#include <stdlib.h>

#include "list.h"
#include "../memory/block.h"

#define tree(type)	tree_create(sizeof(type))

#define tree_iterate(t, item, depth)		\
int depth;									\
for(void *item = tree_first(t, &depth);		\
	!tree_islast(t);				\
	item = tree_next(t, &depth))

#define NO_DEPTH_LIMIT	(0xFFFF)	//no way a tree would go deeper than this..... right??

typedef enum
{
	TRAVERSE_DFS,
	TRAVERSE_DFS_EULERTOUR,
	TRAVERSE_BFS,
	//TRAVERSE_IDDFS,
	//TRAVERSE_IDDFS_EULERTOUR,

	INVALID_TRAVERSE_MODES
} tree_traverse_modes;

typedef struct tnode_s
{
	struct tnode_s **children;
	uint8_t child_ct;
	float score;
	uint8_t move_index;
	void *data;
} tnode_t;

typedef struct
{
	size_t item_size;
	uint32_t node_ct;
	list_t *search_list;
	tree_traverse_modes traverse_mode;	//DFS by default
	int search_depth;		//counter for iterative deepening
	tnode_t *p, *head;
	int depth;
	bool first_visit;

	//void *node_allocator, *data_allocator;

	void (*destroy_fp)(void *d);
	int (*print_fp)(void *d);
	int (*compare_fp)(const void *a, const void *b);

} tree_t;



/////////////////// protos //////////////////

//creates a tree
tree_t *tree_create(size_t size);

////// iterator functions //////
void *tree_first(tree_t *t, int *depth);
void *tree_next(tree_t *t, int *depth);
bool tree_islast(tree_t *t);
bool tree_is_leaf(tree_t *t);
void tree_set_traverse_mode(tree_t *t, tree_traverse_modes mode);
void *tree_get_head(tree_t *t);
void *tree_get_child(tree_t *t, int n);
tnode_t *tree_get(tree_t *t, tnode_t *n);
void tree_add(tree_t *t, void *data);
void tree_add_copies(tree_t *t, int cnt);
void tree_add_empties(tree_t *t, int cnt);
void tree_destroy(tree_t *t);
void tree_clear(tree_t *t);
void tree_delete_all_but_first(tree_t *t, int n);
void tree_delete_child(tree_t *t, int n);
void tree_draw(tree_t *t, int maxdepth);
void tree_set_search_depth(tree_t *t, int d);
void tree_clear_search_depth(tree_t *t);
void tree_set_score(tree_t *t, float score);
float tree_get_score(tree_t *t);
void tree_sort_children(tree_t *t);
void tree_swap_children(tree_t *t, int a, int b);

void tree_attach_print_fn(tree_t *t, int (*print_fp)(void *d));
void tree_attach_destroy_fn(
	tree_t *t, void (*destroy_fp)(void *d));
void tree_attach_compare_fn(
	tree_t *t, int (*compare_fp)(const void *a, const void *b));

void tree_demo(void);

int compare_by_score_ascending(const void *a, const void *b);
int compare_by_score_descending(const void *a, const void *b);


#endif	//TREE_H_
