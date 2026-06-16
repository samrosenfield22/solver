

#ifndef LIST_H_
#define LIST_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define list(type)	list_create(sizeof(type))
#define list_iterate(ll, d)	\
	for(void *d=list_first(ll); !list_islast(ll); d=list_next(ll))
#define list_push(ll, d)	list_append(ll, d)
#define list_enq(ll, d)		list_append(ll, d)
#define list_peek(ll)		list_last(ll)


typedef struct node_s
{
	struct node_s *prev, *next;
	void *data;
} lnode_t;

//typedef struct node_s lnode_t;

typedef struct
{
	//meta
	size_t item_size;
	uint32_t len;

	void (*destroy_fp)(void *d);
	void (*print_fp)(void *d);

	//list
	lnode_t *prev, *p, *next;
	lnode_t *head, *last;
} list_t;

//////////////////////// protos ///////////////////////

////// create/destroy lists, list elements

//creates and returns a list
//use the list() macro, pass the data type
list_t *list_create(size_t item_size);

//destroys a list, including all items if a destroy method is attached
void list_destroy(list_t *ll);

//deletes all items without destroying the list
void list_clear(list_t *ll);

//deletes the current item
void list_delete(list_t *ll);

//////stack/queue funcs
void *list_pop(list_t *ll);
void *list_deq(list_t *ll);

//////access list elements
//////(list_iterate is usually the best way to do this)

//gets first item
void *list_first(list_t *ll);

//gets next item
void *list_next(list_t *ll);

//returns true if current item is last
bool list_islast(list_t *ll);

//gets last item
void *list_last(list_t *ll);

//gets nth item
void *list_nth(list_t *ll, uint32_t n);


//returns number of items
uint32_t list_len(list_t *ll);

//returns true if list has 0 items
bool list_isempty(list_t *ll);

//adds item to end of list
void list_append(list_t *ll, void *data);

//inserts item after current item
void list_insert(list_t *ll, void *data);

//prints list
void list_print(list_t *ll);

//prints list address and length
void list_print_meta(list_t *ll);

//attaches custom callback
void list_attach_print_fn(list_t *ll, void (*print_fp)(void *d));
void list_attach_destroy_fn(list_t *ll, void (*destroy_fp)(void *d));

void list_demo(void);

#endif	//LIST_H_
