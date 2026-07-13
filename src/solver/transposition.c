

#include "transposition.h"

#include "../utils/utils.h"
//#include "play_windows.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <immintrin.h>

#define TRANSTBL_BUCKETS_CT (1<<26)


//statics
tt_t *tt_make(uint32_t len);
//void tt_enable_multithread(void);
void tt_add_kvpair(tt_t *h, void *key, trans_value_t *value, uint64_t hash);
bool tt_key_get_value(tt_t *h, void *key,
	trans_value_t *value, uint64_t hash);
void tt_attach_hash(uint64_t (*hash)(void *key, size_t size));
void tt_attach_replace(bool (*replace_transpose)(void *old, void *new));
uint32_t tt_key_get_index(tt_t *h, void *key, uint64_t *hash);
uint64_t kv_unlock(kvpair_t *kv);
//bool tt_replace_by_depth(void *old, void *new);
//bool tt_replace_by_ancient(void *old, void *new);


tt_t *trans_tbl = NULL;


void tt_create(void)
{
	if(trans_tbl)
		return;
	assert(sizeof(kvpair_t) == 16);
	//printf("%d\n", sizeof(trans_value_t));
	//exit(0);

	//create the table
	trans_tbl = tt_make(TRANSTBL_BUCKETS_CT);
	if(!trans_tbl)
	{
		printf("failed to allocate transposition table\n");
		exit(0);
	}
	//if(MULTICORE_CT > 1)
	//	tt_enable_multithread();
	if(solver->hash)
		tt_attach_hash(solver->hash);
	//tt_attach_replace(REPLACE_STRATEGY);
}

void tt_destroy(void)
{
	//tt_clear();
	/*if(trans_tbl->multithread)
	{
		for(int i=0; i<TT_LOCK_CT; i++)
			omp_destroy_lock(&trans_tbl->locks[i]);
	}*/
	mem_free(trans_tbl);
	trans_tbl = NULL;
}

void tt_clear(void)
{
	if(!trans_tbl)
		return;

	for(uint32_t i=0; i<trans_tbl->len; i++)
	{
		bucket_t *bucket = &trans_tbl->map[i];
		bucket->deeper_kv.value.value_filled = 0b0;
		bucket->always_kv.value.value_filled = 0b0;
		/*if(trans_tbl->map[i])
		{
			void *kv = trans_tbl->map[i];

			//free_kvpair(kv);
			if(kv)
			{
				mem_free(kv);
				trans_tbl->map[i] = NULL;
			}

			trans_tbl->filled--;
		}*/
	}

	trans_tbl->filled = 0;
	//trans_tbl->collisions = 0;
}

/*void tt_set_ancient(void)
{
	if(!trans_tbl)
		return;
	if(trans_tbl->replace_fp != tt_replace_by_ancient)
		return;

	for(uint32_t i=0; i<trans_tbl->len; i++)
	{
		//if(trans_tbl->map[i])
		bucket_t *bucket = &trans_tbl->map[i];
		if(bucket->deeper_kv.value.value_filled)
		//if(trans_tbl->map[i].value.value_filled)
		{
			//void *kv = trans_tbl->map[i];
			//trans_value_t *v = tt_kv_get_value(trans_tbl, kv);
			//v->ancient = true;

			//trans_tbl->map[i].value.ancient = 0b1;
			bucket->deeper_kv.value.ancient = 0b1;
		}
		if(bucket->always_kv.value.value_filled)
			bucket->always_kv.value.ancient = 0b1;
	}
}*/

void tt_add(void *pos, uint64_t *hp, result_t *result, int search_depth, int bound, int best_move)
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
		.value_filled = 0b1,
		//.ancient = 0b0,
		.search_depth = search_depth,
		.best_move = best_move,
	};


	tt_add_kvpair(trans_tbl, pos, &value, hash);
}

//trans_value_t *tt_get(gdata_t *gd, int depth)
bool tt_get(trans_value_t *value, gdata_t *gd, int depth)
{
	void *pos = &(gd->pos);
	uint64_t *hash = gdata_get_hash(gd);
	bool got = tt_key_get_value(trans_tbl, pos, value, *hash);


	if(!got && solver->flip && depth<=solver->flip_depth)
	{
		uint8_t flipped[solver->pos_size];
		solver->flip(flipped, pos);
		uint64_t temp_hash = trans_tbl->hash(flipped, 8);

		got = tt_key_get_value(trans_tbl, flipped, value, temp_hash);

		//catch_pos(pos);
		//printf("%s\n", got? "got flip" : "no flip");
		//catch_pos(flipped);
		//assert(!got);
	}


	//return value;
	return got;
}


/////////////////////////////////////////////


//1 (old val) gets replaced with 2 (new val)
/*bool tt_replace_by_depth(void *old, void *new)
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
}*/

tt_t *tt_make(uint32_t len)
{
	tt_t *h = mem_calloc((len * sizeof(trans_tbl->map[0])) + sizeof(*h), 1);
	if(!h)
		return NULL;

	//h->ksize = ksize;
	//h->vsize = vsize;
	h->len = len;
	//h->collisions = 0;
	h->filled = 0;
	h->hash = NULL;
	//h->multithread = false;

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


	return h;
}

/*void tt_enable_multithread(void)
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
}*/

void tt_add_kvpair(tt_t *h, void *key, trans_value_t *value,
	uint64_t hash)
{
	if(!h)
	{
		printf("you forgot to tt_create()!\n");
		exit(0);
	}

	uint32_t index = tt_key_get_index(h, key, &hash);
	bucket_t *bucket = &h->map[index];

	//lockless
	kvpair_t passed_kv = {.hash=hash, .value=*value};
	uint64_t hash_xor = kv_unlock(&passed_kv);
	//uint64_t hash_xor = hash ^ *(uint64_t *)value;
	//uint64_t deeper_hash = bucket->deeper_kv.hash ^ *(uint64_t *)(&bucket->deeper_kv.value);
	//uint64_t always_hash = bucket->always_kv.hash ^ *(uint64_t *)(&bucket->always_kv.value);

	/*
	if either kv isn't filled, or has same hash key, add
	else if greater depth than deeper, replace
	else replace always
	*/
	if(!bucket->deeper_kv.value.value_filled)
	{
		//bucket->deeper_kv.hash = hash_xor;
		//bucket->deeper_kv.value = *value;
		bucket->deeper_kv.raw = (__int128)_mm_set_epi64x(*(uint64_t*)value, hash);
		trans_tbl->filled++;
	}
	//else if(deeper_hash == hash)
	else if(kv_unlock(&bucket->deeper_kv) == hash)
		bucket->deeper_kv.value = *value;
	else if(!bucket->always_kv.value.value_filled)
	{
		//bucket->always_kv.hash = hash_xor;
		//bucket->always_kv.value = *value;
		bucket->always_kv.raw = (__int128)_mm_set_epi64x(*(uint64_t*)value, hash_xor);
		trans_tbl->filled++;
	}
	//else if(always_hash == hash)
	else if(kv_unlock(&bucket->always_kv) == hash)
		bucket->always_kv.value = *value;
	else if(bucket->deeper_kv.value.search_depth < value->search_depth)
		//&& value->bound == BOUND_EXACT)
	{
		//bucket->deeper_kv.hash = hash_xor;
		//bucket->deeper_kv.value = *value;
		bucket->deeper_kv.raw = (__int128)_mm_set_epi64x(*(uint64_t*)value, hash_xor);
	}
	else
	{
		//bucket->always_kv.hash = hash_xor;
		//bucket->always_kv.value = *value;
		bucket->always_kv.raw = (__int128)_mm_set_epi64x(*(uint64_t*)value, hash_xor);
	}
	return;

}

void tt_prefetch(uint64_t hash)
{
	uint32_t index = tt_key_get_index(trans_tbl, NULL, &hash);
	__builtin_prefetch(&trans_tbl->map[index], 0, 0);
}

bool tt_key_get_value(tt_t *h, void *key,
	trans_value_t *value, uint64_t hash)
{
	if(!h)
	{
		printf("you forgot to tt_create()!\n");
		exit(0);
	}

	uint32_t index = tt_key_get_index(h, key, &hash);
	bucket_t *bucket = &h->map[index];
	//kvpair_t *kv = &bucket->deeper_kv;

	//uint64_t deeper_hash = bucket->deeper_kv.hash ^ *(uint64_t *)(&bucket->deeper_kv.value);
	//uint64_t always_hash = bucket->always_kv.hash ^ *(uint64_t *)(&bucket->always_kv.value);

	kvpair_t *kv = NULL;
	if(bucket->deeper_kv.value.value_filled
		//&& (hash == deeper_hash))
		&& (hash == kv_unlock(&bucket->deeper_kv)))
		kv = &bucket->deeper_kv;
	else if(bucket->always_kv.value.value_filled
		//&& (hash == always_hash))
		&& (hash == kv_unlock(&bucket->always_kv)))
		kv = &bucket->always_kv;
	else
	//if(!kv)
		return false;

	//check lockless integrity
	/*
	//uint64_t hash_xor = kv->hash ^ *(uint64_t *)(&kv->value);
	kvpair_t passed_kv = {.hash=hash, .value=*value};
	uint64_t hash_xor = kv_unlock(&passed_kv);
	//assert(hash_xor == hash);
	if(hash_xor != hash)
		return false;*/

	//if(kv)
	//{
		*value = kv->value;
		return true;
	/*}
	else
		return false;*/
}

//returns the load factor out of 100
int tt_load(void)
{
	uint64_t f = trans_tbl->filled;
	return (f * 50) / trans_tbl->len;	//2 kv per bucket
}

/*uint32_t tt_collisions(void)
{
	return trans_tbl->collisions;
}*/

void tt_attach_hash(uint64_t (*hash)(void *value, size_t size))
{
	if(!trans_tbl)
		return;

	trans_tbl->hash = hash;
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

uint32_t tt_key_get_index(tt_t *h, void *key, uint64_t *hash)
{
	return tt_avalanche(*hash) & h->p2_mask;

	/*uint64_t index;
	if(hash)
		index = (*hash);
	else
	{
		assert(0);
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

	*/
}

/*uint64_t kv_lock(uint64_t hash, uint64_t value)
{
	return hash ^ value;
}

bool kv_lock_good(kvpair_t *kv)
{
	return ((kv->hash ^ kv->value) == kv->value)
}*/

uint64_t kv_unlock(kvpair_t *kv)
{
	trans_value_t *tv = &kv->value;
	uint64_t v = *(uint64_t *)tv;
	//assert((v & 0xFF) == 0);
	//printbig(v, "%x");
	//getchar();
	return kv->hash ^ v;

	//return kv->hash ^ *(uint64_t*)&(kv->value);
}
