

#include "play.h"

#include "games/nim.h"
#include "games/ttt.h"
#include "games/c4.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


#define COMP_TIME	(10 * 1000)
#define DEV_MODE	(true)
//#define DEV_MODE	(false)

void print_sequence(int8_t *seq);
bool load_pgn(solver_t *solver, void *pos, char *in);
bool read_pgn_file(solver_t *solver, void *pos, char *name);
bool seq_charp_to_ints(solver_t *solver, void *pos, char *seq);
void seq_add(int move);

int8_t seq[200] = {-1};
int seq_ct = 0;

void play_menu(void)
{
	srand(time(NULL));

	/*bool mode_selected = false;
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
	}*/


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

	printf("enter pgn, or press enter for a new game:\n");
	char pgn[640];
	fgets(pgn, 639, stdin);
	void *pos = game->initial_pos;
	bool valid = load_pgn(game, pos, pgn);
	if(!valid)
		return;
	//void *starting_pos = construct_pos(game, movelist);

	play(game, pos, COMPUTER_PLAYER, HUMAN_PLAYER);

	/*if(playing)
	{
		//if(rand() & 0b1)
		//	play(game, NULL, HUMAN_PLAYER, COMPUTER_PLAYER);
		//else
			play(game, NULL, COMPUTER_PLAYER, HUMAN_PLAYER);
	}
	else
		solve(game, NULL, 0);*/
}

void play(solver_t *solver, void *pos, bool p1, bool p2)
{
	int move;

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
			print_sequence(seq);
			printf("\n\nenter your move:   ");
			char buf[80];
			fgets(buf, 79, stdin);

			//if(strcmp(buf, "u")==0 || strcmp(buf, "undo")==0)

			//int hmove;
			if(solver->human_to_iter)
				move = solver->human_to_iter(buf);
			else
				move = strtol(buf, NULL, 10);
			if(move >= solver->possible_moves
				|| !solver->is_legal(pos, move))
			{
				printf("illegal move!\n");
				continue;
			}
			solver->make_move(pos, move, NULL);

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

			move = solve(solver, pos, COMP_TIME, DEV_MODE);
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
			print_sequence(seq);
			break;
		}

		turn = !turn;

		seq_add(move);
	}
}

bool load_pgn(solver_t *solver, void *pos, char *in)
{
	//void *pos = solver->initial_pos;
	bool valid;
	if(in[strlen(in)-1] == '\n')
		in[strlen(in)-1] = '\0';
	if(strstr(in, ".txt"))
		valid = read_pgn_file(solver, pos, in);
	else
		valid = seq_charp_to_ints(solver, pos, in);
	return valid;
}

bool read_pgn_file(solver_t *solver, void *pos, char *name)
{
	//make file name
	const char *dir = "algos/solver/pgns";
	char buf[640];
	snprintf(buf, 639, "%s/%s", dir, name);

	//open file
	FILE *fp = fopen(buf, "r");
	if(!fp)
	{
		printf("invalid file %s\n", buf);
		return false;
	}

	//load moves
	fgets(buf, 639, fp);
	return seq_charp_to_ints(solver, pos, buf);
}

bool seq_charp_to_ints(solver_t *solver, void *pos, char *seq)
{
	for(char *token = strtok(seq, ","); token;
		token = strtok(NULL, ","))
	{
		if(*token == '\n')
			return true;
		int move = strtol(token, NULL, 10);
		printf("move=%d\n", move);

		if(solver->is_legal(pos, move))
		{
			solver->make_move(pos, move, NULL);
			seq_add(move);
		}
		else
		{
			printf("illegal move %d\n", move);
			return false;
		}
	}

	return true;
}

void print_sequence(int8_t *seq)
{
	printf("\n\nmoves played: ");
	for(int8_t *move = seq; *move!=-1; move++)
	{
		printf("%d, ", *move);
	}
	printf("\n");
}

void seq_add(int move)
{
	seq[seq_ct] = move;
	seq_ct++;
	seq[seq_ct] = -1;
}
