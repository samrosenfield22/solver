

#include "block.h"
#include "alloc.h"

#include <assert.h>


//statics
block_t *block_create(b_alloc_t *ba);
void block_insert(b_alloc_t *ba, block_t *b);

b_alloc_t *block_allocator(size_t size, int init)
{
	b_alloc_t *ba = mem_malloc(sizeof(*ba));
	assert(ba);
	ba->size = size + sizeof(block_t);

	ba->first = NULL;
	//ba->last = NULL;

	for(int i=0; i<init; i++)
		block_create(ba);

	return ba;
}

void *block(b_alloc_t *ba)
{
	block_t *b = ba->first;
	if(b)
		ba->first = b->next;
	else
		b = block_create(ba);

	return b->data;
}

void block_free(b_alloc_t *ba, void *data)
{
	block_t *b = data;
	b--;
	block_insert(ba, b);
}

/////////////// statics //////////////

block_t *block_create(b_alloc_t *ba)
{
	block_t *b = mem_malloc(ba->size);
	assert(b);

	//insert at head
	block_insert(ba, b);

	return b;
}

//insert at head
void block_insert(b_alloc_t *ba, block_t *b)
{
	b->next = ba->first;
	ba->first = b;
}
