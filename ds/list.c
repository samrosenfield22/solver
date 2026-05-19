

#include "list.h"
#include "../utils.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

#define USE_SLAB_ALLOC

//static protos
void list_dummy_print_node(void *d);
lnode_t *list_create_node(int size, void *data);
lnode_t *list_create_node_empty(void);
void *list_set_current_node(list_t *ll, lnode_t *n);

void lnode_free(lnode_t *n);

list_t *list_create(size_t item_size)
{
	list_t *ll = mem_malloc(sizeof(*ll));
	if(!ll)
		return NULL;

	ll->item_size = item_size;
	ll->len = 0;
	ll->destroy_fp = NULL;
	ll->print_fp = list_dummy_print_node;
	ll->p = NULL;
	ll->prev = NULL;
	ll->next = NULL;
	ll->head = NULL;
	ll->last = NULL;

	return ll;
}

void list_destroy(list_t *ll)
{
	if(!ll)
		return;

	list_clear(ll);

	mem_free(ll);
}

void list_clear(list_t *ll)
{
	if(!ll)
		return;

	list_iterate(ll, d)
	{
		list_delete(ll);
		(void)d;
	}

	assert(list_len(ll) == 0);
}

void list_delete(list_t *ll)
{
	//printf("deleting...\n");

	if(!ll)
		return;

	if(!ll->p)
		return;

	if(ll->destroy_fp)
		ll->destroy_fp(ll->p->data);
	//

	//printf("shouldnt have had a destroy fp...\n");

	//links
	//printf("%p\n", ll->prev);
	//printf("%d\n", *(int*)ll->prev->data);
	//if(ll->prev)
	//	ll->prev->next = ll->next;
	if(ll->p->prev)	ll->p->prev->next = ll->p->next;
	if(ll->p->next)	ll->p->next->prev = ll->p->prev;
	//ll->next = ll->p->next;

	//printf("linked prev to next\n");

	if(ll->p == ll->head)
		ll->head = ll->p->next;
	if(ll->p == ll->last)
		ll->last = ll->p->prev;


	//if(ll->p->data)
	//	mem_free(ll->p->data);

	lnode_free(ll->p);
	//mem_free(ll->p);
	ll->p = NULL;

	ll->len--;
}


void *list_pop(list_t *ll)
{
	void *popped = list_last(ll);
	list_delete(ll);
	return popped;
}


//void list_enq(list_t *ll, void *item)
void *list_deq(list_t *ll)
{
	void *deq = list_first(ll);
	list_delete(ll);
	return deq;
}

void *list_first(list_t *ll)
{
	if(!ll)
		return NULL;

	return list_set_current_node(ll, ll->head);
	//ll->p = ll->head;
	//return ll->p->data;
}

void *list_next(list_t *ll)
{
	if(!ll)
		return NULL;


	return list_set_current_node(ll, ll->next);


	/*ll->p = ll->p->next;
	if(ll->p)
		return ll->p->data;
	else
		return NULL;*/
}

bool list_islast(list_t *ll)
{
	if(!ll)
		return true;

	return ll->p==NULL? true : false;
}

void *list_last(list_t *ll)
{
	if(!ll)
		return NULL;

	return list_set_current_node(ll, ll->last);
	/*ll->p = ll->last;
	if(ll->p)
		return ll->p->data;
	else
		return NULL;*/
}

void *list_nth(list_t *ll, uint32_t n)
{
	if(n >= ll->len)
		return NULL;

	uint32_t i=0;
	list_iterate(ll, d)
	{
		if(i == n)
			return d;

		i++;
	}

	assert(0);
	return NULL;
}

uint32_t list_len(list_t *ll)
{
	return ll->len;
}

bool list_isempty(list_t *ll)
{
	return (ll->len == 0);
}



void list_append(list_t *ll, void *data)
{
	if(!ll)
		return;

	//create the node
	lnode_t *n = list_create_node(ll->item_size, data);
	if(!n)
		return;

	//add to list
	if(ll->len)
	{
		ll->last->next = n;
		n->prev = ll->last;
	}
	else
	{
		ll->head = n;
	}
	ll->last = n;

	ll->len++;
}

void list_insert(list_t *ll, void *data)
{
	if(!ll)
		return;

	if(!ll->p || ll->p == ll->last)
	{
		list_append(ll, data);
		return;
	}

	//create the node
	lnode_t *n = list_create_node(ll->item_size, data);
	if(!n)
		return;


	//update head and last ptrs
	/*if(!ll->p)
	{
		ll->head = n;
	}
	if(ll->p == ll->last)
	{
		ll->last = n;
	}*/

	//add to list: p -> n -> next
	if(ll->p)
	{
		ll->p->next = n;
		n->prev = ll->p;
	}
	n->next = ll->next;

	ll->prev = ll->p;
	ll->p = n;

	ll->len++;
}

void list_print(list_t *ll)
{
	if(!ll)
		return;

	if(ll->len == 0)
	{
		printf("(empty)\n");
		return;
	}

	if(!ll->print_fp)
		return;

	int i = 0;
	list_iterate(ll, d)
	{
		ll->print_fp(d);
		printf(", ");

		i++;
		if(i > ll->len + 3)
			break;
	}
	printf("\n");
}

void list_print_meta(list_t *ll)
{
	if(ll)
		printf("list at %p with %d elements\n", ll, ll->len);
	else
		printf("NULL list pointer\n");
}

void list_attach_print_fn(list_t *ll, void (*print_fp)(void *d))
{
	if(!ll)
		return;

	ll->print_fp = print_fp;
}

void list_attach_destroy_fn(
	list_t *ll, void (*destroy_fp)(void *d))
{
	if(!ll)
		return;

	ll->destroy_fp = destroy_fp;
}

void list_demo(void)
{
	list_t *mylist = list(int);

	//add some items to the list
	list_append(mylist, &(int){7});
	list_append(mylist, &(int){8});
	list_append(mylist, &(int){22});
	list_append(mylist, &(int){67});
	list_print(mylist);

	list_destroy(mylist);
	return;

	/*
	//double it
	list_iterate(mylist, item)
	{
		*(int*)item *= 2;
	}
	list_print(mylist);
	*/

	/*int search = 2;
	printf("element #%d is %d\n",
		search, *(int*)list_nth(mylist, search));

	printf("first elem is %d\n", *(int*)list_first(mylist));
	printf("last elem is %d\n", *(int*)list_last(mylist));*/

	list_iterate(mylist, item)
	{
		printf("found elem %d\n", *(int*)item);
		if(*(int *)item == 67)
		{
			//list_delete(mylist);
			//printf("\tdeleting!\n");
			list_insert(mylist, &(int){55});
		}
	}
	printf("last elem is %d\n", *(int*)list_last(mylist));

	list_print(mylist);
	list_print_meta(mylist);

	list_clear(mylist);
	list_print(mylist);
}

//////////////////////// static functions //////////////////////

void list_dummy_print_node(void *d)
{
	printf("%d", *(int *)d);
}

lnode_t *list_create_node(int size, void *data)
{
	lnode_t *n = list_create_node_empty();
	n->data = mem_malloc(size);
	if(!n->data)
	{
		lnode_free(n);
		//mem_free(n);
		n = NULL;
	}
	else
		memcpy(n->data, data, size);
	return n;
}

lnode_t *list_create_node_empty(void)
{
	#ifdef USE_SLAB_ALLOC
	lnode_t *n = sl_alloc(sizeof(*n));
	#else
	lnode_t *n = mem_malloc(sizeof(*n));
	#endif

	if(!n)
		return NULL;
	n->prev = NULL;
	n->next = NULL;
	return n;
}

void *list_set_current_node(list_t *ll, lnode_t *n)
{
	void *node_data = NULL;

	ll->prev = ll->p;
	ll->p = n;

	if(ll->p)
	{
		ll->next = ll->p->next;
		//return ll->p->data;
		node_data = ll->p->data;
	}
	else
	{
		ll->next = NULL;
		//return NULL;
	}

	//
	//if(ll->prev)	assert(ll->prev->next == ll->p);
	if(ll->p)		assert(ll->p->next == ll->next);

	return node_data;
}

void lnode_free(lnode_t *n)
{
	#ifdef USE_SLAB_ALLOC
	sl_free(n);
	#else
	mem_free(n);
	#endif
}
