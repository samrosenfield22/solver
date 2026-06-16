

#ifndef TRANSPOSITION_H_
#define TRANSPOSITION_H_

#include <stdint.h>
#include <stdbool.h>

#include <omp.h>

#include "shared.h"


/*typedef struct
{
	//void *key;
	//void *value;
	uint8_t data[];
} kvpair_t;*/

typedef struct
{
	size_t ksize, vsize;//, kvsize;
	uint32_t len, filled;
	uint32_t collisions;
	int p2_mask;

	uint32_t (*hash)(void *key, size_t size);
	bool (*compare_keys_fp)(void *k1, void *k2);
	void (*normalize_key)(void *k);
	bool (*replace_fp)(void *k1, void *v1, void *k2, void *v2);

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
	uint8_t bound;
	uint8_t best_move;
	//age??
} trans_value_t;

enum
{
	TT_NO_ADD,
	TT_ADDED_KV_NEW,
	TT_KEYS_MATCHED_REPLACED_VALUE,
	TT_POLICY_OVERWRITE_KV,
	TT_POLICY_DENIED_KV,
} TT_blahlbah;


/////////////////////// protos ///////////////////////
void tt_create(size_t ksize, uint32_t len);
void tt_destroy(void);
void tt_clear(void);
int tt_add(void *pos, uint32_t *hash, result_t *result,
	int search_depth, int bound, int best_move);
bool tt_get(trans_value_t *value, gdata_t *gd, int depth);

int tt_load(void);
uint32_t tt_collisions(void);

#endif	//TRANSPOSITION_H_
