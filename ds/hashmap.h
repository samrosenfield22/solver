

#include "list.h"

#define hashmap(ktype, vtype, len)		\
	hashmap_create(sizeof(ktype), sizeof(vtype), len)

typedef struct
{
	void *key;
	void *value;
} kvpair_t;

typedef struct
{
	size_t ksize, vsize;
	int len, filled;

	uint32_t (*hash)(void *key, size_t size);
	bool (*compare_keys_fp)(void *k1, void *k2);
	void (*normalize_key)(void *k);
	bool (*replace_fp)(void *k1, void *v1, void *k2, void *v2);

	void *map[];
} hashmap_t;


/////////////////////// protos ///////////////////////
hashmap_t *hashmap_create(size_t ksize, size_t vsize, uint32_t len);
void hashmap_destroy(hashmap_t *h);
void hashmap_clear(hashmap_t *h);
void hashmap_add_kvpair(hashmap_t *h, void *key, void *value);
void *hashmap_key_get_value(hashmap_t *h, void *key);
int hashmap_load(hashmap_t *h);
void hashmap_attach_hash(
	hashmap_t *h, uint32_t (*hash)(void *key, size_t size));
void hashmap_attach_keycompare(hashmap_t *h,
	bool (*compare_keys_fp)(void *k1, void *k2));
void hashmap_attach_normalize(hashmap_t *h,
	void (*normalize_key)(void *k));
void hashmap_attach_replace(hashmap_t *h,
	bool (*replace_transpose)(void *k1, void *v1,
	void *k2, void *d1));
void hashmap_demo(void);
