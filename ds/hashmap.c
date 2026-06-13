
#include "../utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define HM_LOCK_CT	(10000)

//statics
//void *copy_key_normalize(hashmap_t *h, void *key);
void *hashmap_key_get_bucket(hashmap_t *h, void *key, uint32_t *hash);
uint32_t hashmap_key_get_index(hashmap_t *h, void *key, uint32_t *hash);
//void *pairlist_find_matching_key(hashmap_t *h,
//	list_t *pairlist, void *key);
bool keys_match(hashmap_t *h, void *k1, void *k2);
kvpair_t *alloc_kvpair(hashmap_t *h);
void free_kvpair(kvpair_t *kv);

uint32_t default_hash(void *key, size_t size);
void hashmap_set_lock(hashmap_t *h, int index);
void hashmap_unset_lock(hashmap_t *h, int index);


hashmap_t *hashmap_create(size_t ksize, size_t vsize, uint32_t len)
{
	hashmap_t *h = mem_calloc((len * sizeof(void *)) + sizeof(*h), 1);
	if(!h)
		return NULL;

	h->ksize = ksize;
	h->vsize = vsize;
	h->len = len;
	h->collisions = 0;
	h->filled = 0;
	h->hash = default_hash;
	h->compare_keys_fp = NULL;
	h->normalize_key = NULL;
	h->multithread = false;

	h->p2_mask = 0;
	uint32_t b = 0b1;
	for(int i=0; i<32; i++)
	{
		if(b == len)
		{
			h->p2_mask = b-1;
			break;
		}
		else if(b > len)
			break;
		b<<=1;
	}
	//printf("len=%d, mask=0x%0x\n", len, h->p2_mask);
	//exit(0);


	return h;
}

void hashmap_destroy(hashmap_t *h)
{
	hashmap_clear(h);
	if(h->multithread)
	{
		for(int i=0; i<HM_LOCK_CT; i++)
			omp_destroy_lock(&h->locks[i]);
	}
	mem_free(h);
}

void hashmap_clear(hashmap_t *h)
{
	if(!h)
		return;

	for(uint32_t i=0; i<h->len; i++)
	{
		if(h->map[i])
		{
			kvpair_t *kv = h->map[i];

			free_kvpair(kv);
			h->map[i] = NULL;

			h->filled--;
		}
	}

	//h->filled = 0;
	h->collisions = 0;
}

void hashmap_enable_multithread(hashmap_t *h)
{
	if(!h || h->multithread)
		return;

	//printf("\n\n\n\n\ninitializing hashmap multithread for\n%u locks... ",
	//	h->len);

	h->multithread = true;
	h->locks = mem_malloc(h->len * sizeof(*h->locks));
	//printf("\nalloc done, initing locks... ");
	for(uint32_t i=0; i<HM_LOCK_CT; i++)
	{
		omp_init_lock(&h->locks[i]);
	}
	//printf("done!\n");
}

int hashmap_add_kvpair(hashmap_t *h, void *key, void *value,
	uint32_t *hash)
{
	if(!h)
		return HM_NO_ADD;

	int add_result = -1;

	//void *norm_key = copy_key_normalize(h, key);

	//kvpair_t **bucket = hashmap_key_get_bucket(h, key, hash);
	uint32_t index = hashmap_key_get_index(h, key, hash);
	kvpair_t **bucket = &h->map[index];
	assert(bucket);

	if(h->multithread)
		hashmap_set_lock(h, index);
		//omp_set_lock(&h->locks[index]);

	kvpair_t *kv;
	if(*bucket)	//bucket already filled
	{
		kv = *bucket;

		if(keys_match(h, kv->key, key))
		{
			//replace value
			memcpy(kv->value, value, h->vsize);
			add_result = HM_KEYS_MATCHED_REPLACED_VALUE;
		}
		else	//collision, apply replacement policy
		{
			h->collisions++;
			bool replace = true;
			if(h->replace_fp)
				replace = h->replace_fp(kv->key, kv->value, key, value);

			if(replace)
			{
				memcpy(kv->key, key, h->ksize);
				memcpy(kv->value, value, h->vsize);
				add_result = HM_POLICY_OVERWRITE_KV;
			}
			else
				add_result = HM_POLICY_DENIED_KV;
		}

	}
	else	//bucket empty, add the kv pair
	{
		kv = alloc_kvpair(h);
		*bucket = kv;


		//printf("moving %d bytes from %p to %p\n", h->ksize, key, kv->key);
		memcpy(kv->key, key, h->ksize);
		memcpy(kv->value, value, h->vsize);

		//printf("kvpair added!\n");
		h->filled++;
		add_result = HM_ADDED_KV_NEW;
	}

	if(h->multithread)
		hashmap_unset_lock(h, index);
		//omp_unset_lock(&h->locks[index]);
	return add_result;

	////////////////////////////////////////////////////////


	/*
	//get address of pairlist
	list_t **pairlist_p = hashmap_key_get_bucket(h, key);

	//make new list if the bucket is empty
	//printf("%p\n", *pairlist_p);
	if(*pairlist_p == NULL)
	{
		//printf("made new pairlist\n");
		*pairlist_p = list(kvpair_t);
		if(!(*pairlist_p))
		{
			printf("failed to make list!\n");
			exit(0);
		}
	}


	kvpair_t *kv = pairlist_find_matching_key(h, *pairlist_p, key);
	if(kv)
	{
		//printf("collision! overwriting value\n\n");
		memcpy(kv->value, value, h->vsize);
	}
	else
	{
		//make pair, add to map
		//printf("\tadding new pair (%d,%d)\n",
		//	*(int*)key, *(int*)value);
		kvpair_t pair = {.key=key, .value=value};
		list_append(*pairlist_p, &pair);
	}

	assert(memcmp(hashmap_key_get_value(h, key), value, h->vsize)==0);
	*/
}

void *hashmap_key_get_value(hashmap_t *h, void *key, uint32_t *hash)
{
	if(!h)
		return NULL;


	//printf("getting bucket\n");
	//kvpair_t **bucket = hashmap_key_get_bucket(h, key, hash);
	uint32_t index = hashmap_key_get_index(h, key, hash);
	kvpair_t **bucket = &h->map[index];
	assert(bucket);
	//printf("\tgot bucket\n");
	if(!bucket)
		return NULL;
	kvpair_t *kv = *bucket;
	if(!kv)	//slot not filled
		return NULL;

	if(h->multithread)
		hashmap_set_lock(h, index);
		//omp_set_lock(&h->locks[index]);


	//return kv->value;
	bool match = keys_match(h, kv->key, key);

	void *val = match? kv->value : NULL;

	if(h->multithread)
		hashmap_unset_lock(h, index);
		//omp_unset_lock(&h->locks[index]);
	return val;

	/*if(match)
		return kv->value;
	else
		return NULL;*/


	/////////////////////////////////////////////
	/*
	list_t *pairlist = *(list_t**)hashmap_key_get_bucket(h, key);


	kvpair_t *kv = pairlist_find_matching_key(h, pairlist, key);
	//printf("\tobtained kv");
	if(kv)
	{
		printf("\treturning value %d\n", *(int16_t*)kv->value);
		return kv->value;
	}
	else
	{
		//printf("no matching key found for key %d!\n", *(int*)key);
		return NULL;
	}
	*/
}

//returns the load factor out of 100
int hashmap_load(hashmap_t *h)
{
	//printbig(h->filled, "%d");
	//printf(" out of %s buckets filled\n",
	//	sprintbig(h->len, "%d"));
	return (h->filled * 100) / h->len;
}

uint32_t hashmap_collisions(hashmap_t *h)
{
	return h->collisions;
}



void hashmap_attach_hash(
	hashmap_t *h, uint32_t (*hash)(void *value, size_t size))
{
	if(!h)
		return;

	h->hash = hash;
}

void hashmap_attach_keycompare(hashmap_t *h,
	bool (*compare_keys_fp)(void *k1, void *k2))
{
	if(!h)
		return;

	h->compare_keys_fp = compare_keys_fp;
}

void hashmap_attach_normalize(hashmap_t *h,
	void (*normalize_key)(void *k))
{
	if(!h)
		return;

	h->normalize_key = normalize_key;
}

void hashmap_attach_replace(hashmap_t *h,
	bool (*replace_fp)(void *k1, void *v1,
	void *k2, void *d1))
{
	if(!h)
		return;

	h->replace_fp = replace_fp;
}



void hashmap_demo(void)
{
	//add (7,88)
	hashmap_t *h = hashmap(int, int, 100);
	hashmap_add_kvpair(h, &(int){7}, &(int){88}, NULL);
	int got = *(int*)hashmap_key_get_value(h, &(int){7}, NULL);
	printf("got %d\n", got);

	//overwrite key 7 with 6767
	hashmap_add_kvpair(h, &(int){7}, &(int){6767}, NULL);
	got = *(int*)hashmap_key_get_value(h, &(int){7}, NULL);
	printf("got %d\n", got);


	hashmap_add_kvpair(h, &(int){107}, &(int){55}, NULL);
	got = *(int*)hashmap_key_get_value(h, &(int){7}, NULL);
	printf("got %d\n", got);
	got = *(int*)hashmap_key_get_value(h, &(int){107}, NULL);
	printf("got %d\n", got);

	hashmap_destroy(h);
}

////////////////////////// statics ///////////////////////////

//have to mem_free it
/*void *copy_key_normalize(hashmap_t *h, void *key)
{
	void *norm_key = mem_malloc(h->ksize);
	if(!norm_key)
	{
		printf("failed to allocate for normalization!\n");
		assert(0);
		return NULL;
	}
	memcpy(norm_key, key, h->ksize);
	if(h->normalize_key)
		h->normalize_key(norm_key);
	return norm_key;
}*/

uint32_t avalanche(uint32_t index)
{
	uint64_t x = index;

	x *= 0x61664b66ad5f0385ull;
	x ^= x >> 32;
	x *= 0xf959d19084fd5339ull;
	x ^= x >> 32;
	return x;
}

void *hashmap_key_get_bucket(hashmap_t *h, void *key, uint32_t *hash)
{
	if(!h)
		return NULL;

	uint32_t index = hashmap_key_get_index(h, key, hash);

	return &h->map[index];
	//list_t **pairlist = (list_t**)&h->map[index];
	//return pairlist;
}

uint32_t hashmap_key_get_index(hashmap_t *h, void *key, uint32_t *hash)
{
	/*if(!h)
		return 0;


	uint32_t index = h->hash(key, h->ksize);
	//index %= h->len;

	return index;*/



	uint32_t index;
	if(hash)
		index = (*hash);
	else
	{
		index = h->hash(key, h->ksize);
		index %= h->len;
	}
	index = avalanche(index);

	if(h->p2_mask)
		index &= h->p2_mask;
	else
		index %= h->len;

	return index;
}

/*void *pairlist_find_matching_key(hashmap_t *h,
	list_t *pairlist, void *key)
{
	assert(h);
	assert(key);
	if(!pairlist)
		return NULL;

	//printf("pairlist has length %d\n", list_len(pairlist));
	list_iterate(pairlist, item)
	{
		kvpair_t *kv = item;
		//if(memcmp(key, kv->key, h->ksize)==0)
		if(keys_match(h, key, kv->key))
		{
			//printf("found kv (%d,%d)\n", *(int*)kv->key, *(int*)kv->value);
			return kv;
		}
	}

	//no matching key found!
	return 0;
}*/

bool keys_match(hashmap_t *h, void *k1, void *k2)
{
	if(h->compare_keys_fp)
		return h->compare_keys_fp(k1, k2);
	else
		return (memcmp(k1, k2, h->ksize)==0);
}

kvpair_t *alloc_kvpair(hashmap_t *h)
{
	kvpair_t *kv = mem_malloc(sizeof(*kv));
	assert(kv);
	kv->key = mem_malloc(h->ksize);
	kv->value = mem_malloc(h->vsize);
	assert(kv->key);
	assert(kv->value);

	return kv;
}

void free_kvpair(kvpair_t *kv)
{
	if(kv->key)
		mem_free(kv->key);
	if(kv->value)
		mem_free(kv->value);
	mem_free(kv);
}

uint32_t default_hash(void *key, size_t size)
{
	uint32_t hash = 0;
	uint8_t *p = (uint8_t*)key;
	for(int i=0; i<size; i++)
	{
		//hash += *p++;
		hash ^= *p;
		p++;
		hash <<= 1;
	}
	return hash;
}

uint32_t default_hash_string(void *key, size_t size)
{
	uint32_t hash = 0;

	for(char *p = *(char**)key; *p; p++)
	{
		//hash += *p++;
		hash ^= *p;
		p++;
		hash <<= 1;
	}
	return hash;
}

void hashmap_set_lock(hashmap_t *h, int index)
{
	omp_lock_t *lock = &h->locks[index / HM_LOCK_CT];
	omp_set_lock(lock);
}

void hashmap_unset_lock(hashmap_t *h, int index)
{
	omp_lock_t *lock = &h->locks[index / HM_LOCK_CT];
	omp_unset_lock(lock);
}

/*
//set_create() is a #define
set_include()
set_has()
*/
