

#ifndef BLOCK_H_
#define BLOCK_H_

#include <stdint.h>

typedef struct block_s
{
	struct block_s *next;
	uint8_t data[];
} block_t;

typedef struct
{
	block_t *first;//, *last;
	size_t size;
} b_alloc_t;

b_alloc_t *block_allocator(size_t size, int init);
void block_allocator_destroy(b_alloc_t *ba);
void *block(b_alloc_t *ba);
void block_free(b_alloc_t *ba, void *data);

#endif	//BLOCK_H_
