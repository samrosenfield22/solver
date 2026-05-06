

#ifndef SOLVER_H_
#define SOLVER_H_


#include <stdint.h>
#include <stdbool.h>

#define USE_ALPHABETA_PRUNING
#define USE_TRANSPOSITION_TABLE
//#define CLEAR_SUB_NODES

#define VARIATION_LENGTH	(4)
#define PRINCIPAL_VAR_CT	(20)
#define INNER_VAR_CT		(7)

/*typedef struct
{
	bool whosemove;

	void *pos;
} gamestate_t;*/

typedef enum
{
	END_NOT_OVER = 0,
	END_P1_WON = 1,
	END_P2_WON = 2,
	END_DRAW = 3,
} endstate_t;

#define WIN_SCORE	(100.0)
//#define INF			(10000.0)
#define INF		(100.0)

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
	void (*make_move)(void *pos, int index);
	uint32_t (*hash)(void *key, size_t size);
	bool (*keys_match)(void *k1, void *k2);
	void (*normalize_position)(void *k);
	bool (*replace_transpose)(void *k1, void *k2);

	int (*print_pos)(void *pos);

	void (*draw_full)(void *pos);
	int (*human_to_iter)(char *human);
	char *(*iter_to_human)(int move);

} solver_t;

float solve(solver_t *solver, void *pos);


#endif	//SOLVER_H_
