

#ifndef TRANSPOSITION_H_
#define TRANSPOSITION_H_

#include <stdint.h>
#include <stdbool.h>

#include <omp.h>

#include "shared.h"

//#define REPLACE_STRATEGY	(NULL)	//always replace
#define REPLACE_STRATEGY	(tt_replace_by_depth)
//#define REPLACE_STRATEGY	(tt_replace_by_ancient)

/*typedef struct
{
	uint64_t hash;
	trans_value_t value;
} kvpair_t;

typedef struct
{
	kvpair_t first, second;
} bucket_t;
*/

typedef struct
{
	size_t ksize, vsize;//, kvsize;
	uint32_t len, filled;
	uint32_t collisions;
	uint32_t p2_mask;

	uint64_t (*hash)(void *key, size_t size);
	bool (*compare_keys_fp)(void *k1, void *k2);
	bool (*replace_fp)(void *old, void *new);

	bool multithread;
	omp_lock_t *locks;

	//kvpair_t *map[];
	void *map[];
	//uint8_t map[];
} tt_t;

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

	uint8_t bound	: 2;
	uint8_t ancient	: 1;
	//age??
} trans_value_t;

enum
{
	TT_NO_ADD,
	TT_ADDED_KV_NEW,
	TT_KEYS_MATCHED_REPLACED_VALUE,
	TT_POLICY_OVERWRITE_KV,
	TT_POLICY_DENIED_KV,
};


/////////////////////// protos ///////////////////////
void tt_create(size_t ksize, uint32_t len);
void tt_destroy(void);
void tt_clear(void);
void tt_set_ancient(void);
int tt_add(void *pos, uint64_t *hash, result_t *result,
	int search_depth, int bound, int best_move);
bool tt_get(trans_value_t *value, gdata_t *gd, int depth);

int tt_load(void);
uint32_t tt_collisions(void);

#endif	//TRANSPOSITION_H_
