

#include "transposition.h"

#include "../utils/utils.h"
//#include "play_windows.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define TT_LOCK_CT	(1000000)

//statics
tt_t *tt_make(size_t ksize, size_t vsize, uint32_t len);
void tt_enable_multithread(void);
int tt_add_kvpair(tt_t *h, void *key, void *value, uint64_t hash);
bool tt_key_get_value(tt_t *h, void *key,
	void *value, uint64_t hash);
void tt_attach_hash(uint64_t (*hash)(void *key, size_t size));
//void tt_attach_keycompare(bool (*compare_keys_fp)(void *k1, void *k2));
void tt_attach_replace(bool (*replace_transpose)(void *old, void *new));

void *tt_key_get_bucket(tt_t *h, void *key, uint64_t *hash);
uint32_t tt_key_get_index(tt_t *h, void *key, uint64_t *hash);

//bool tt_keys_match(tt_t *h, void *k1, void *k2);
//kvpair_t *alloc_kvpair(tt_t *h);
//void free_kvpair(kvpair_t *kv);
void *tt_kv_get_value(tt_t *h, void *kv);
void tt_set_lock(tt_t *h, uint32_t index);
bool tt_try_lock(tt_t *h, uint32_t index);
void tt_unset_lock(tt_t *h, uint32_t index);

bool tt_replace_by_depth(void *old, void *new);
bool tt_replace_by_ancient(void *old, void *new);


tt_t *trans_tbl = NULL;


void tt_create(uint32_t len)
{
	if(trans_tbl)
		return;

	//create the table
	trans_tbl = tt_make(sizeof(uint64_t),
		sizeof(trans_value_t),
		len);
	if(!trans_tbl)
	{
		printf("failed to allocate transposition table\n");
		exit(0);
	}
	if(MULTICORE_CT > 1)
		tt_enable_multithread();
	if(solver->hash)
		tt_attach_hash(solver->hash);
	//if(solver->keys_match)
		//tt_attach_keycompare(solver->keys_match);
	tt_attach_replace(REPLACE_STRATEGY);
}

void tt_destroy(void)
{
	tt_clear();
	if(trans_tbl->multithread)
	{
		for(int i=0; i<TT_LOCK_CT; i++)
			omp_destroy_lock(&trans_tbl->locks[i]);
	}
	mem_free(trans_tbl);
	trans_tbl = NULL;
}

void tt_clear(void)
{
	if(!trans_tbl)
		return;

	for(uint32_t i=0; i<trans_tbl->len; i++)
	{
		if(trans_tbl->map[i])
		{
			void *kv = trans_tbl->map[i];

			//free_kvpair(kv);
			if(kv)
			{
				mem_free(kv);
				trans_tbl->map[i] = NULL;
			}

			trans_tbl->filled--;
		}
	}

	//h->filled = 0;
	trans_tbl->collisions = 0;
}

void tt_set_ancient(void)
{
	if(!trans_tbl)
		return;
	if(trans_tbl->replace_fp != tt_replace_by_ancient)
		return;

	for(uint32_t i=0; i<trans_tbl->len; i++)
	{
		if(trans_tbl->map[i])
		{
			void *kv = trans_tbl->map[i];
			trans_value_t *v = tt_kv_get_value(trans_tbl, kv);
			v->ancient = true;
		}
	}
}

int tt_add(void *pos, uint64_t *hp, result_t *result, int search_depth, int bound, int best_move)
{
	assert(best_move >= 0);

	//void *pos = &(gd->pos);
	//uint64_t *hash = gdata_get_hash(gd);
	//int search_depth = iddfs_depth - depth;

	uint64_t hash;
	if(hp)
		hash = *hp;
	else
		hash = trans_tbl->hash(pos, 0);

	trans_value_t value =
	{
		.score = result->score,
		.full = result->full,
		.bound = bound,
		.ancient = false,
		.search_depth = search_depth,
		.best_move = best_move,
	};


	return tt_add_kvpair(trans_tbl, pos, &value, hash);
}

//trans_value_t *tt_get(gdata_t *gd, int depth)
bool tt_get(trans_value_t *value, gdata_t *gd, int depth)
{
	void *pos = &(gd->pos);
	uint64_t *hash = gdata_get_hash(gd);
	bool got = tt_key_get_value(trans_tbl, pos, value, *hash);


	/*if(!got && solver->flip && depth<=solver->flip_depth)
	{
		uint8_t flipped[solver->pos_size];
		solver->flip(flipped, pos);
		uint64_t temp_hash = trans_tbl->hash(flipped, trans_tbl->ksize);

		got = tt_key_get_value(trans_tbl, flipped, value, &temp_hash);

		//catch_pos(pos);
		//printf("%s\n", got? "got flip" : "no flip");
		//catch_pos(flipped);
		//assert(!got);
	}*/


	//return value;
	return got;
}


/////////////////////////////////////////////


//1 (old val) gets replaced with 2 (new val)
bool tt_replace_by_depth(void *old, void *new)
{
	//return false;

	trans_value_t *val_old = old;
	trans_value_t *val_new = new;

	return (val_new->search_depth >= val_old->search_depth);
}

bool tt_replace_by_ancient(void *old, void *new)
{
	trans_value_t *val_old = old;
	trans_value_t *val_new = new;

	if(val_new->search_depth >= val_old->search_depth)
		return true;
	if(val_new->search_depth + 16 < val_old->search_depth)
		return false;

	return (val_old->ancient);
	//return (val_new->search_depth >= val_old->search_depth);
}

tt_t *tt_make(size_t ksize, size_t vsize, uint32_t len)
{
	tt_t *h = mem_calloc((len * sizeof(void *)) + sizeof(*h), 1);
	if(!h)
		return NULL;

	h->ksize = ksize;
	h->ksize += 4 - (ksize%4);
	h->vsize = vsize;
	//h->vsize += 4 - (vsize%4);
	h->len = len;
	h->collisions = 0;
	h->filled = 0;
	h->hash = NULL;
	//h->compare_keys_fp = NULL;
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

void tt_enable_multithread(void)
{
	if(!trans_tbl || trans_tbl->multithread)
		return;

	//printf("\n\n\n\n\ninitializing hashmap multithread for\n%u locks... ",
	//	h->len);

	trans_tbl->multithread = true;
	trans_tbl->locks = mem_malloc(trans_tbl->len * sizeof(*trans_tbl->locks));
	//printf("\nalloc done, initing locks... ");
	for(uint32_t i=0; i<TT_LOCK_CT; i++)
	{
		omp_init_lock(&trans_tbl->locks[i]);
	}
	//printf("done!\n");
}

int tt_add_kvpair(tt_t *h, void *key, void *value,
	uint64_t hash)
{
	if(!h)
	{
		printf("you forgot to tt_create()!\n");
		exit(0);
	}

	int add_result = -1;

	//kvpair_t **bucket = tt_key_get_bucket(h, key, hash);
	uint32_t index = tt_key_get_index(h, key, &hash);
	void **bucket = &h->map[index];
	//assert(bucket);

	if(h->multithread)
	{
		//printf("getting lock... ");
		//tt_set_lock(h, index);
		//printf("got!\n");
		if(!tt_try_lock(h, index))
			return TT_NO_ADD;
	}

	void *kv;
	if(*bucket)	//bucket already filled
	{
		kv = *bucket;
		void *val_p = tt_kv_get_value(h, kv);

		//if(keys_match(h, kv->key, key))
		//if(tt_keys_match(h, kv, key))	//key is the first elem of kvpair
		//if(tt_keys_match(h, kv, &hash))	//key is the first elem of kvpair
		if(hash == *(uint64_t *)kv)
		{
			//replace value
			memcpy(val_p, value, h->vsize);
			add_result = TT_KEYS_MATCHED_REPLACED_VALUE;
		}
		else	//collision, apply replacement policy
		{
			h->collisions++;
			bool replace = true;
			if(h->replace_fp)
			{
				//replace = h->replace_fp(kv->key, kv->value, key, value);
				replace = h->replace_fp(val_p, value);
			}

			if(replace)
			{
				memcpy(kv, &hash, h->ksize);
				memcpy(val_p, value, h->vsize);
				add_result = TT_POLICY_OVERWRITE_KV;
			}
			else
				add_result = TT_POLICY_DENIED_KV;
		}

	}
	else	//bucket empty, add the kv pair
	{
		kv = mem_malloc(h->ksize + h->vsize);
		*bucket = kv;

		void *val_p = tt_kv_get_value(h, kv);
		memcpy(kv, &hash, h->ksize);
		memcpy(val_p, value, h->vsize);

		//printf("kvpair added!\n");
		h->filled++;
		add_result = TT_ADDED_KV_NEW;
	}

	if(h->multithread)
		tt_unset_lock(h, index);
		//omp_unset_lock(&h->locks[index]);
	return add_result;
}

bool tt_key_get_value(tt_t *h, void *key,
	void *value, uint64_t hash)
{
	if(!h)
	{
		printf("you forgot to tt_create()!\n");
		exit(0);
	}
	//if(!h->multithread)
	//	printf("no multi!\n");

	uint32_t index = tt_key_get_index(h, key, &hash);
	void **bucket = &h->map[index];
	/*assert(bucket);
	if(!bucket)
		return false;*/
	void *kv = *bucket;
	if(!kv)	//slot not filled
		return false;

	if(h->multithread)
	{
		if(!tt_try_lock(h, index))
			return false;
	}

	//bool match = tt_keys_match(h, kv, &hash);
	bool match = (hash == *(uint64_t *)kv);
	//void *val = match? kv->value : NULL;
	if(match)
	{
		//memcpy(value, kv->value, h->vsize);
		trans_value_t *val_p = tt_kv_get_value(h, kv);
		memcpy(value, val_p, h->vsize);

		if(trans_tbl->replace_fp == tt_replace_by_ancient)
			val_p->ancient = false;
	}

	if(h->multithread)
		tt_unset_lock(h, index);
	return match;
}

//returns the load factor out of 100
int tt_load(void)
{
	uint64_t f = trans_tbl->filled;
	return (f * 100) / trans_tbl->len;
}

uint32_t tt_collisions(void)
{
	return trans_tbl->collisions;
}

void tt_attach_hash(uint64_t (*hash)(void *value, size_t size))
{
	if(!trans_tbl)
		return;

	trans_tbl->hash = hash;
}

/*void tt_attach_keycompare(bool (*compare_keys_fp)(void *k1, void *k2))
{
	if(!trans_tbl)
		return;

	trans_tbl->compare_keys_fp = compare_keys_fp;
}*/

void tt_attach_replace(bool (*replace_fp)(void *old, void *new))
{
	if(!trans_tbl)
		return;

	trans_tbl->replace_fp = replace_fp;
}



////////////////////////// statics ///////////////////////////



uint64_t tt_avalanche(uint64_t index)
{
	uint64_t x = index;

	x *= 0x61664b66ad5f0385ull;
	x ^= x >> 32;
	x *= 0xf959d19084fd5339ull;
	x ^= x >> 32;
	return x;
}

void *tt_key_get_bucket(tt_t *h, void *key, uint64_t *hash)
{
	if(!h)
		return NULL;

	uint32_t index = tt_key_get_index(h, key, hash);

	return &h->map[index];
}

uint32_t tt_key_get_index(tt_t *h, void *key, uint64_t *hash)
{

	uint64_t index;
	if(hash)
		index = (*hash);
	else
	{
		assert(0);
		//printf("no hash\n");
		index = h->hash(key, h->ksize);
		//index %= h->len;
		if(h->p2_mask)
			index &= h->p2_mask;
		else
			index %= h->len;
	}
	index = tt_avalanche(index);

	if(h->p2_mask)
		index &= h->p2_mask;
	else
		index %= h->len;

	return index;
}

/*bool tt_keys_match(tt_t *h, void *k1, void *k2)
{
	uint64_t *u1 = k1;
	uint64_t *u2 = k2;
	return (*u1 == *u2);

	//if(h->compare_keys_fp)
	//	return h->compare_keys_fp(k1, k2);
	//else
	//	return (memcmp(k1, k2, h->ksize)==0);
}*/

/*kvpair_t *alloc_kvpair(tt_t *h)
{
	kvpair_t *kv = mem_malloc(sizeof(*kv));
	assert(kv);
	kv->key = mem_malloc(h->ksize);
	kv->value = mem_malloc(h->vsize);
	assert(kv->key);
	assert(kv->value);

	return kv;
}*/

/*void free_kvpair(kvpair_t *kv)
{
	if(kv->key)
		mem_free(kv->key);
	if(kv->value)
		mem_free(kv->value);
	mem_free(kv);
}*/


void *tt_kv_get_value(tt_t *h, void *kv)
{
	char *vp = kv;
	vp += h->ksize;
	return vp;
}

void tt_set_lock(tt_t *h, uint32_t index)
{
	omp_lock_t *lock = &h->locks[index / TT_LOCK_CT];
	omp_set_lock(lock);
}

bool tt_try_lock(tt_t *h, uint32_t index)
{
	omp_lock_t *lock = &h->locks[index / TT_LOCK_CT];
	return omp_test_lock(lock);
}

void tt_unset_lock(tt_t *h, uint32_t index)
{
	omp_lock_t *lock = &h->locks[index / TT_LOCK_CT];
	omp_unset_lock(lock);
}
