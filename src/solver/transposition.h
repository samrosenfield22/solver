

#ifndef TRANSPOSITION_H_
#define TRANSPOSITION_H_

#include <stdint.h>
#include <stdbool.h>

#include <stdatomic.h>
#include <omp.h>

#include "shared.h"

typedef unsigned __int128 uint128_t;
typedef _Atomic uint128_t atomic_uint128_t;

//#define REPLACE_STRATEGY	(NULL)	//always replace
#define REPLACE_STRATEGY	(tt_replace_by_depth)
//#define REPLACE_STRATEGY	(tt_replace_by_ancient)

enum
{
	BOUND_EXACT,
	BOUND_UPPER,
	BOUND_LOWER,
};

typedef struct
{
	float score;
	uint8_t search_depth;
	bool full;
	uint8_t best_move;

	uint8_t value_filled	: 1;
	uint8_t bound	: 2;
	uint8_t ancient	: 1;
	//age??
} trans_value_t;

typedef union
{
	struct
	{
		uint64_t hash;
		trans_value_t value;
	};
	atomic_uint128_t raw;
} kvpair_t;

typedef struct
{
	kvpair_t deeper_kv, always_kv;
} bucket_t;


typedef struct
{
	size_t ksize, vsize;//, kvsize;
	uint32_t len, filled;
	uint32_t collisions;
	uint32_t p2_mask;

	uint64_t (*hash)(void *key, size_t size);
	//bool (*compare_keys_fp)(void *k1, void *k2);
	bool (*replace_fp)(void *old, void *new);

	bool multithread;
	omp_lock_t *locks;

	//kvpair_t map[];
	//void *map[];
	//uint8_t map[];
	bucket_t map[];
} tt_t;

enum
{
	TT_NO_ADD,
	TT_ADDED_KV_NEW,
	TT_KEYS_MATCHED_REPLACED_VALUE,
	TT_POLICY_OVERWRITE_KV,
	TT_POLICY_DENIED_KV,
};


/////////////////////// protos ///////////////////////
void tt_create(uint32_t len);
void tt_destroy(void);
void tt_clear(void);
void tt_set_ancient(void);
int tt_add(void *pos, uint64_t *hp, result_t *result,
	int search_depth, int bound, int best_move);
bool tt_get(trans_value_t *value, gdata_t *gd, int depth);

int tt_load(void);
uint32_t tt_collisions(void);

#endif	//TRANSPOSITION_H_
