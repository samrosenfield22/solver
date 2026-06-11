

#ifndef PLAY_H_
#define PLAY_H_

#include "solver.h"

//#define FORCE_MOVE_ITERATION

enum
{
	GAME_UNFINISHED,
	WIN_BY_CHECKMATE,
	WIN_BY_TIMEOUT,
	DRAW_BY_STALEMATE,
	//by agreement, insufficient material
};

typedef struct
{
	float score;	//0, 1/2, 1
	bool winner;	//false=p1, true=p2
	int reason;
	void *pos;
} game_outcome_t;

enum
{
	HUMAN_PLAYER = true,
	COMPUTER_PLAYER = false,
};

void play_menu(void);
game_outcome_t play(solver_t *solver, void *pos, bool p1, bool p2);


#endif	//PLAY_H_
