

#ifndef SOLVER_H_
#define SOLVER_H_


#include <stdint.h>
#include <stdbool.h>

#define USE_ALPHABETA_PRUNING
#define USE_TRANSPOSITION_TABLE
#define CLEAR_SUB_NODES

#define VARIATION_LENGTH	(3)
#define PRINCIPAL_VAR_CT	(7)
#define INNER_VAR_CT		(1)



typedef enum
{
	END_NOT_OVER = 0,
	END_P1_WON = 1,
	END_P2_WON = 2,
	END_DRAW = 3,
} endstate_t;

#define WIN_SCORE	(10000.0)
#define MATE_LIMIT	(WIN_SCORE - 100)
//#define INF			(10000.0)
//#define INF		(100.0)

typedef struct
{
	float score;
	uint8_t best;
	uint8_t iddfs;
	uint8_t depth;
	//ab bounds
	//uint8_t move_index;
	int8_t best_move;
} trans_value_t;

typedef struct
{
	char *name;

	void *initial_pos;
	size_t pos_size;
	int possible_moves;
	uint32_t transtbl_buckets_ct;

	uint8_t *default_order;

	endstate_t (*gameover)(void *pos);
	float (*estimate)(void *pos);
	bool (*whosemove)(void *pos);
	//bool (*is_end)(void *pos);
	bool (*is_legal)(void *pos, int index);
	void (*make_move)(void *pos, int index, uint32_t *hash);
	bool (*move_loses)(void *pos, int move);
	uint32_t (*hash)(void *key, size_t size);
	bool uses_zobrist;
	bool (*keys_match)(void *k1, void *k2);
	//void (*normalize_position)(void *k);
	//bool (*replace_transpose)(void *k1, void *k2);
	bool (*replace_transpose)(void *k1, void *v1,
		void *k2, void *d1);

	int (*print_pos)(void *pos);

	void (*draw_full)(void *pos);
	int (*human_to_iter)(char *human);
	char *(*iter_to_human)(int move);

} solver_t;

float test_pos(solver_t *game_solver, int *seq, int len);
float solve(solver_t *solver, void *pos, int time_lim_ms,
	bool verbose);


#endif	//SOLVER_H_
