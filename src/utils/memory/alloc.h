

#ifndef	ALLOC_H_
#define ALLOC_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

void *mem_malloc(size_t size);
void *mem_calloc(size_t num, size_t size);
void *mem_realloc(void *p, size_t size);
void mem_free(void *p);
bool mem_check(void);
uint64_t mem_count(void);

#endif
