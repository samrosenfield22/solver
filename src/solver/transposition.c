/*

#include "transposition.h"

#include "../../utils.h"
#include "play_windows.h"

void tt_create(void)
{

	//gdata_hash_size = sizeof(gdata_t) + solver->hash_size;

	//printf("tt w ksize=%d, vsize=%d\n", solver->hash_size, sizeof(trans_value_t));
	//exit(0);

	//create the table
	trans_tbl = hashmap_create(solver->hash_size,
		sizeof(trans_value_t),
		solver->transtbl_buckets_ct);
	if(!trans_tbl)
	{
		printf("failed to allocate transposition table\n");
		exit(0);
	}
	if(MULTICORE_CT > 1)
		hashmap_enable_multithread(trans_tbl);
	if(solver->hash)
		hashmap_attach_hash(trans_tbl, solver->hash);
	if(solver->keys_match)
		hashmap_attach_keycompare(trans_tbl,
			solver->keys_match);
	//if(solver->normalize_position)
	//	hashmap_attach_normalize(trans_tbl, solver->normalize_position);
	if(solver->replace_transpose)
		hashmap_attach_replace(trans_tbl, solver->replace_transpose);


}

int tt_add(gdata_t *gd, result_t *result, int depth, int bound, int best_move)
{
	assert(best_move >= 0);

	void *pos = &(gd->pos);
	uint32_t *hash = gdata_get_hash(gd);
	int search_depth = iddfs_depth - depth;

	trans_value_t value =
	{
		.score = result->score,
		.full = result->full,
		.bound = bound,
		//.iddfs = iddfs_depth,
		//.depth = depth,
		.search_depth = search_depth,
		.best_move = best_move,
	};


	return hashmap_add_kvpair(trans_tbl, pos, &value, hash);
}

//trans_value_t *tt_get(gdata_t *gd, int depth)
bool tt_get(trans_value_t *value, gdata_t *gd, int depth)
{
	void *pos = &(gd->pos);
	uint32_t *hash = gdata_get_hash(gd);
	//trans_value_t *value = hashmap_key_get_value(trans_tbl, pos, hash);
	bool got = hashmap_key_get_value(trans_tbl, pos, value, hash);

	//if(!value && solver->flip && depth<=solver->flip_depth)
	if(!got && solver->flip && depth<=solver->flip_depth)
	{
		uint8_t flipped[solver->pos_size];
		solver->flip(flipped, pos);
		//value = hashmap_key_get_value(trans_tbl, flipped, NULL);
		got = hashmap_key_get_value(trans_tbl, flipped, value, NULL);
	}


	//return value;
	return got;
}

void draw_tt_load(void)
{
	const int load_bar_len = 25;
	const int load_bar_h = 16;
	const int load_num_h = load_bar_h - 3;

	static bool first = true;
	if(first)
	{
		first = false;
		window_unfocus();
		term_move_cursor(0, load_num_h);
		printf("0%%");
		term_move_cursor(0, load_bar_h-1);
		printf("^");
		term_move_cursor(0, load_bar_h+load_bar_len);
		printf("v");
		window_focus(analysis_hdl);
	}

	static int last_load = 0;
	int load = hashmap_load(trans_tbl);
	if(load == last_load)
		return;
	last_load = load;

	printf(TERM_WHITE);
	window_unfocus();
	term_move_cursor(0, load_bar_h);
	for(int i=0; i<load/4; i++)
		printf("%c\n", 219);

	term_move_cursor(0, load_num_h);
	printf("%d%% ", load);

	window_focus(analysis_hdl);
}
*/
