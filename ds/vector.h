

#ifndef VECTOR_H_
#define VECTOR_H_

#include <stdint.h>

#define vector(type, len)	vector_create(sizeof(type), len)
#define vector_resize(vp, len)	\
	vector_resize_internal((void **)vp, len)

typedef struct
{
	uint32_t len, item_size;
	void (*destroy_fp)(void *d);
} vector_hdr_t;

//////////////////////////// protos //////////////////////////
void *vector_create(size_t item_size, uint32_t len);
void vector_destroy(void *vv);
void vector_resize_internal(void **vp, uint32_t len);

void vector_print_meta(void *vh);
void vector_attach_destroy_fn(
	void *vv, void (*destroy_fp)(void *d));

#endif	//VECTOR_H_
