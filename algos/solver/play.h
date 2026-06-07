

#ifndef PLAY_H_
#define PLAY_H_

#include "solver.h"

//#define FORCE_MOVE_ITERATION

enum
{
	HUMAN_PLAYER = true,
	COMPUTER_PLAYER = false,
};

void play_menu(void);
void play(solver_t *solver, void *pos, bool p1, bool p2);


#endif	//PLAY_H_
