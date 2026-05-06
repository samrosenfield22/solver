

#include "alloc.h"

uint64_t ALLOC_CT = 0;

void *mem_malloc(size_t size)
{
	void *p = malloc(size);
	if(p)
		ALLOC_CT++;
	return p;
}
void *mem_calloc(size_t num, size_t size)
{
	void *p = calloc(num, size);
	if(p)
		ALLOC_CT++;
	return p;
}

void *mem_realloc(void *p, size_t size)
{
	void *new_p = realloc(p, size);
	return new_p;
}

void mem_free(void *p)
{
	if(p)
	{
		free(p);
		ALLOC_CT--;
	}
}

bool mem_check(void)
{
	return (ALLOC_CT==0);
}

uint64_t mem_count(void)
{
	return ALLOC_CT;
}
