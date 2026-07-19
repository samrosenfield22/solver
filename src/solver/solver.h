#ifndef SOLVER_H_
#define SOLVER_H_

#include <stdint.h>
#include <stdbool.h>

#define USE_ALPHABETA_PRUNING
#define USE_TRANSPOSITION_TABLE
//#define RETURN_FIRST_WIN_FOUND
//#define USE_HISTORY_HEURISTIC
#define ASPIRATION_WINDOW
#define USE_SYMMETRY
#define PRINCIPAL_VAR_SEARCH
#define USE_LATE_MOVE_REDUCTIONS

//not working yet
#define MULTICORE_CT	(1)

//#define FORCE_SEARCH_DEPTH	(32)
extern int FORCE_SEARCH_DEPTH;

//how many full moves are shown in each variation
#define VARIATION_LENGTH	(4)

//number of variations -- if greater than 1, analysis will
//take longer
#define DISPLAY_VAR_CT		(1)


typedef enum
{
	END_NOT_OVER = 0,
	END_P1_WON = 1,
	END_P2_WON = 2,
	END_DRAW = 3,
} endstate_t;

#define WIN_SCORE	(10000.0)
#define MATE_LIMIT	(WIN_SCORE - 100)

typedef struct
{
	int move;
	float score;
} sorter_t;

typedef struct
{
	char *name;

	/*required:
	points to the game start position, which you must define*/
	void *initial_pos;

	/*required:
	number of bytes of the position structure
	sizeof(my_initial_pos)*/
	size_t pos_size;

	/*required:
	maximum value for move iteration
	i.e. 9 for tictactoe, 7 for connect 4*/
	int possible_moves;

	/*required:
	how many iddfs iterations get skipped
	set it to 1 to start, then play around with making
	it bigger*/
	int iddfs_increment;

	/*optional:
	*/
	float aspiration_default_width;

	/*optional: if checking for symmetrical positions in
	the transposition table, only check for symmetries up
	to this depth*/
	int flip_depth;

	/*optional:
	int array of move indices, in order from best guess to worst
	ex. for connect 4, this would be:
	.default_order = (uint8_t[]){3, 2, 4, 1, 5, 0, 6}
	(central moves are better than flank moves)*/
	uint8_t *default_order;

	/*
	optional: for bitmap init, any procedural generation,
	etc
	*/
	void (*init)(void);

	/*required:
	checks if the game is a win for either player, draw,
	or is still in progress*/
	endstate_t (*gameover)(void *pos);

	/*optional:*/
	float (*estimate)(void *pos);

	/*optional:*/
	//float (*estimate_sort)(void *pos, int move);

	/*required:
	returns true for p1, false for p2*/
	bool (*whosemove)(void *pos);


	/*required:
	returns true if the move is legal*/
	bool (*is_legal)(void *pos, int index);

	/*required:
	makes the given move, updating the position in place.
	this assumes that the move is legal.
	if using an updating hash system like zobrist, we can pass
	a pointer to the current hash to be updated. if not, pass
	NULL.*/
	void (*make_move)(void *pos, int index, uint64_t *hash);

	void (*make_move_temp)(void *made, void *pos, int index, uint64_t *hash);

	//optional
	int (*make_movelist)(sorter_t *sorter, void *pos);

	bool (*move_loses)(void *pos, int move);

	/*optional:
	populates the sorter_t array with one or more only
	moves that should be played, and returns the number
	of moves.
	if there is a move that wins immediately, return it
	if not,
	if there are only a few moves that block opponent's
	immediate win, play it
	if not, returns 0 (no only moves)*/
	int (*only_moves)(sorter_t *onlies, void *pos);

	//optional
	bool (*win_impossible_for_current)(void *pos);

	/*required:
	computes the hash. in updating hash systems like zobrist,
	this is still required (currently) for the initial pos
	(although it doesn't really need to be)
	(maybe it does tho, still used for flip pos)*/
	uint64_t (*hash)(void *key, size_t size);

	int (*moves_remaining)(void *pos);

	int (*get_extension)(void *pos);


	bool uses_zobrist;


	/*optional:
	creates a symmetrically equivalent version of the position*/
	void (*flip)(void *to, void *from);

	int (*flip_move_index)(int index);


	/*required:
	draws the position, make it nice looking*/
	void (*draw_full)(void *pos, int last_move);

	void *(*menu_define)(void);
	int (*menu_update)(void *menu, void *pos, int key);

	/*optional:
	converts a human-friendly expression of a move (i.e. Nc3)
	into a move index
	should return -1 if the move is invalid*/
	int (*human_to_iter)(char *human);

	/*optional:
	converts a move index into a human-friendly expression
	of a move (i.e. Nc3)*/
	char *(*iter_to_human)(int move);

} solver_t;

//float test_pos(solver_t *game_solver, int *seq, int len);
//void *construct_pos(solver_t *game_solver, char *seq);
void solver_init(solver_t *game_solver);
void solver_clear(void);
float solve(solver_t *solver, void *pos, int time_lim_ms,
	bool verbose);
void solver_check(solver_t *s);
void catch_pos(void *pos);

#endif	//SOLVER_H_
