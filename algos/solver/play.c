

#include "play.h"

#include "games/nim.h"
#include "games/ttt.h"
#include "games/c4.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#define COMP_TIME	(20 * 1000)

void play_menu(void)
{
	srand(time(NULL));

	bool mode_selected = false;
	bool playing;
	while(1)
	{
		printf("select a mode ('p' to play, 's' to solve):   ");
		char c = getchar();
		switch(c)
		{
			case 'p':	playing = true; mode_selected = true; break;
			case 's':	playing = false; mode_selected = true; break;
			default:	printf("invalid selection \'%c\'!\n", c);
						fflush(stdin);
						break;
		}
		if(mode_selected)
			break;
	}


	solver_t gamelist[] =
	{
		NIM_SOLVER,
		TTT_SOLVER,
		C4_SOLVER,
	};
	//bool game_selected = false;
	solver_t *game = NULL;
	while(1)
	{
		printf("select a game!\n\n");
		int len = sizeof(gamelist)/sizeof(gamelist[0]);
		int gamechoice;
		for(int i=0; i<len; i++)
		{
			printf("\t%d:\t%s\n", i, gamelist[i].name);
		}
		printf("\nyour choice:\t");
		scanf("%d", &gamechoice);
		fflush(stdin);
		if(0 <= gamechoice && gamechoice < len)
		{
			game = &gamelist[gamechoice];
			break;
		}
		else
			printf("invalid game selection (must be between 0 and %d)!\n",
				len-1);
	}

	if(playing)
	{
		//if(rand() & 0b1)
		//	play(game, NULL, HUMAN_PLAYER, COMPUTER_PLAYER);
		//else
			play(game, NULL, COMPUTER_PLAYER, HUMAN_PLAYER);
	}
	else
		solve(game, NULL, 0);
}

void play(solver_t *solver, void *pos, bool p1, bool p2)
{
	printf("starting %s! %s goes first.\n\n",
		solver->name, (p1==HUMAN_PLAYER)? "human":"computer");

	if(!pos)
		pos = solver->initial_pos;

	bool turn = true;
	while(1)
	{
		bool player = turn? p1 : p2;
		if(player == HUMAN_PLAYER)
		{
			/*
			draw position
			prompt for move, wait until given
			make move, update pos
			*/
			solver->draw_full(pos);
			printf("\n\nenter your move:   ");
			char buf[80];
			fgets(buf, 79, stdin);

			int hmove;
			if(solver->human_to_iter)
				hmove = solver->human_to_iter(buf);
			else
				hmove = strtol(buf, NULL, 10);
			if(hmove >= solver->possible_moves
				|| !solver->is_legal(pos, hmove))
			{
				printf("illegal move!\n");
				continue;
			}
			solver->make_move(pos, hmove, NULL);

			/*solver->draw_full(pos);
			printf("enter to continue   ");
			getchar();*/
		}
		else	//COMPUTER_PLAYER
		{
			/*
			solve()
			print which move was played
			*/

			int move = solve(solver, pos, COMP_TIME);
			solver->make_move(pos, move, NULL);
			//printf("i played: %s\n", solver->iter_to_human(move));
			//
		}

		endstate_t game_end_state = solver->gameover(pos);
		if(game_end_state != END_NOT_OVER)
		{
			printf("game over!\n");
			switch(game_end_state)
			{
				case END_P1_WON:
					printf("player 1 (%s) wins!\n",
						(p1==HUMAN_PLAYER)? "human":"computer");
					break;
				case END_P2_WON:
					printf("player 2 (%s) wins!\n",
						(p2==HUMAN_PLAYER)? "human":"computer");
					break;
				case END_DRAW:		printf("draw!\n");	break;
				case END_NOT_OVER:	break;
			}
			solver->draw_full(pos);
			break;
		}

		turn = !turn;
	}
}
