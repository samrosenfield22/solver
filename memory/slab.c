

#include "slab.h"

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

uint8_t SLABS[MEMSIZE];

#define SLAB_SIZE	(MEMSIZE/SLAB_CT)
#define SLABS_END	(SLABS + MEMSIZE)

typedef struct chunk_s
{
	void *next;
} chunk_t;

typedef struct
{
	size_t size;
	void *head;
} slabman_t;

//intialize w size and head = 0
slabman_t slab_mgrs[SLAB_CT] = {0};

//statics
void carve_slab(slabman_t *mgr, int index, size_t size);
void *unlink_chunk(slabman_t *mgr);
void *nth_slab(int i);


/*void sl_init(void)
{
	for(int i=0; i<SLAB_CT; i++)
	{
		slabman_t *mgr = &slab_mgrs[i];

		mgr->size = 0;
		mgr->head = NULL;
	}
}*/

void *sl_alloc(size_t size)
{
	for(int i=0; i<SLAB_CT; i++)
	{
		slabman_t *mgr = &slab_mgrs[i];

		if(mgr->size == size)	//found the slab w the correct size
		{
			if(mgr->head)
				return unlink_chunk(mgr);
			else
			{
				printf("out of chunks of size %d\n", (int)size);
				assert(0);
				return NULL;
			}
		}
		else if(mgr->size == 0)	//no slab w the size, make one
		{
			carve_slab(mgr, i, size);
			return unlink_chunk(mgr);
		}
	}

	//all slabs have different sizes
	printf("no more slabs!!\n");
	return NULL;
}

void sl_free(void *chunk)
{
	if(!chunk)
		return;
	
	assert(SLABS <= (uint8_t *)chunk
		&& (uint8_t *)chunk <= SLABS_END);

	//find the correct slab
	int index = ((uint8_t *)chunk - SLABS) / SLAB_SIZE;
	//printf("returning chunk to slab %d\n", index);

	//get the slab mgr
	slabman_t *mgr = &slab_mgrs[index];

	//add chunk back to the list
	chunk_t *ch = chunk;
	ch->next = mgr->head;
	mgr->head = ch;
}

/////////////////////////////////

void carve_slab(slabman_t *mgr, int index, size_t size)
{
	mgr->size = size;

	//break slab into chunks
	mgr->head = nth_slab(index);

	int chunk_ct = SLAB_SIZE / size;
	printf("carving slab %d into %d chunks of size %d\n",
		index, chunk_ct, (int)size);
	uint8_t *ch = mgr->head;
	for(int i=0; i<chunk_ct; i++)
	{
		assert((void*)ch < nth_slab(index+1));

		//chunk_t *next = ((uint8_t *)ch) + size;
		uint8_t *next = ch + size;
		((chunk_t *)ch)->next = (i < chunk_ct-1)?
			next : NULL;
		ch = next;
	}
}

void *unlink_chunk(slabman_t *mgr)
{
	//printf("getting chunk of size %d\n", (int)(mgr->size));
	chunk_t *chunk = mgr->head;
	mgr->head = chunk->next;
	return chunk;
}

void *nth_slab(int i)
{
	return &SLABS[i * SLAB_SIZE];
}
