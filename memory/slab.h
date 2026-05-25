

#ifndef SLAB_H_
#define SLAB_H_

#include <stdlib.h>

#define MEMSIZE	(1<<20)
#define SLAB_CT		(4)

//
//void sl_init(void);
void *sl_alloc(size_t size);
void sl_free(void *chunk);

#endif	//SLAB_H_
