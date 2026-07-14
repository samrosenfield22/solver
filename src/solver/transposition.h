

#ifndef TRANSPOSITION_H_
#define TRANSPOSITION_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdalign.h>

//#include <stdatomic.h>
//#include <omp.h>

#include "shared.h"

//typedef unsigned __int128 uint128_t;
//typedef _Atomic uint128_t atomic_uint128_t;

//#define REPLACE_STRATEGY	(NULL)	//always replace
//#define REPLACE_STRATEGY	(tt_replace_by_depth)
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
	//int16_t score;
	uint8_t search_depth;
	//bool full;
	uint8_t best_move;

	uint8_t value_filled	: 1;
	uint8_t full			: 1;
	uint8_t bound			: 2;
	uint8_t is_pv			: 1;
} trans_value_t;

typedef union
{
	struct
	{
		uint64_t hash;
		trans_value_t value;
	};
	__int128 raw;
} kvpair_t;
/*typedef struct
{
	uint64_t hash;
	trans_value_t value;
} kvpair_t;*/

typedef struct
{
	kvpair_t deeper_kv, always_kv;
} bucket_t;


typedef struct
{
	//size_t ksize, vsize;
	uint32_t len, filled;
	//uint32_t collisions;
	uint32_t p2_mask;

	uint64_t (*hash)(void *key, size_t size);

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
void tt_create(void);
void tt_destroy(void);
void tt_clear(void);
//void tt_set_ancient(void);
void tt_add(void *pos, uint64_t *hp, result_t *result,
	int search_depth, int bound, int best_move, bool is_pv);
void tt_prefetch(uint64_t hash);
bool tt_get(trans_value_t *value, gdata_t *gd, int depth);

int tt_load(void);
uint32_t tt_collisions(void);

#endif	//TRANSPOSITION_H_
