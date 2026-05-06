

#include "vector.h"
#include "../memory/alloc.h"

#include <stdio.h>
#include <stdlib.h>

#define VECTOR_MIN_LEN	(4)

//statics
vector_hdr_t *vector_get_hdr(vector_hdr_t *vh);
uint32_t vector_len_p2(uint32_t len);


void *vector_create(size_t item_size, uint32_t len)
{
	len = vector_len_p2(len);
	size_t size = sizeof(vector_hdr_t) + len * item_size;
	vector_hdr_t *v = mem_malloc(size);
	if(!v)
		return NULL;

	v->len = len;
	v->item_size = item_size;
	v->destroy_fp = NULL;

	void *v_data = v + 1;
	return v_data;
}

void vector_destroy(void *vv)
{
	vector_hdr_t *v = vector_get_hdr(vv);
	if(v->destroy_fp)
	{
		for(int i=0; i<v->len; i++)
			v->destroy_fp(vv + i * v->item_size);
	}
	mem_free(v);
}

void vector_resize_internal(void **vp, uint32_t len)
{
	if(!(*vp))
		return;
	vector_hdr_t *v = vector_get_hdr(*vp);

	//get power-of-two lengths, compare (see if resize is necessary)
	//uint32_t old_len_p2 = vector_len_p2(v->len);
	uint32_t new_len = vector_len_p2(len);
	if(new_len == v->len)
	{
		//printf("new len is the same, no resize!\n");
		return;
	}

	//if new len is smaller, need to destroy items
	if(v->destroy_fp && new_len < v->len)
	{
		//printf("destroying items from %d to %d\n", new_len, v->len-1);
		for(int i=new_len; i<v->len; i++)
			v->destroy_fp(*vp + i * v->item_size);
	}

	//mem_reallocate
	size_t size = sizeof(vector_hdr_t) + new_len * v->item_size;
	vector_hdr_t *new_v = mem_realloc(v, size);
	if(!new_v)
		return;

	*vp = new_v + 1;
	new_v->len = new_len;
}

void vector_print_meta(void *vh)
{
	vector_hdr_t *v = vector_get_hdr(vh);
	if(v)
		printf("vector at %p with %d elements\n", v, v->len);
	else
		printf("NULL vector pointer\n");
}

void vector_attach_destroy_fn(
	void *vv, void (*destroy_fp)(void *d))
{
	if(!vv)
		return;
	vector_hdr_t *v = vector_get_hdr(vv);

	v->destroy_fp = destroy_fp;
}


/////////////////////// static functions //////////////////////////

vector_hdr_t *vector_get_hdr(vector_hdr_t *vh)
{
	if(!vh)	return NULL;
	return vh-1;
}

uint32_t vector_len_p2(uint32_t len)
{
	//printf("given len %d\n", len);

	if(len < VECTOR_MIN_LEN)
		return VECTOR_MIN_LEN;

	/*int bits = 0;
	for(uint32_t x=len; x; x>>=1)
	{
		bits++;
	}

	uint32_t len_p2 = 1<<bits;*/

	uint32_t x = len-1;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	uint32_t len_p2 = x + 1;

	//printf("len_p2 = %d\n", len_p2);
	return len_p2;
}
