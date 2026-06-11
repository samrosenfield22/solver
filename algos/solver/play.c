

#include "play.h"
#include "play_windows.h"
#include "../../utils.h"
#include "clock.h"

#include "games/nim.h"
#include "games/ttt.h"
#include "games/c4.h"
#include "games/quoridor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <conio.h>

#define PGN_DIR	"algos/solver/pgns"
#define LAST_PGN_DIR	"algos/solver/pgns/repeat.txt"


#define COMP_TIME	(1 * 1000)
#define DEV_MODE	(true)
//#define DEV_MODE	(false)

void print_sequence(solver_t *solver, int8_t *seq, FILE *stream);
void print_sequence_fancy(solver_t *solver, int8_t *seq, FILE *stream);
bool load_pgn(solver_t *solver, void *pos, char *in);
void save_pgn(solver_t *solver, char *path);
bool read_pgn_file(solver_t *solver, char *name);
void load_seq_string(solver_t *solver, char *seq);
bool make_sequence_moves(solver_t *solver, void *pos);
void seq_add(int move);

int8_t seq[200] = {-1};
int seq_ct = 0, seq_entire = 0;

#define printf(fmt, ...)	window_printf(fmt, ##__VA_ARGS__)


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
		QUOR_SOLVER,
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



	while(1)
	{
		printf("enter pgn, (r) for repeat, or press enter for a new game:\n");
		char pgn[640];
		fgets(pgn, 639, stdin);

		if(strcmp(pgn, "r\n")==0)	//pgn repeat
		{
			FILE *fp = fopen(LAST_PGN_DIR, "r");
			if(!fp)
				return;
			fgets(pgn, 639, fp);
			//printf("pgn repeat: %s\n", pgn);
			//exit(0);
			fclose(fp);
		}
		else if(pgn[0] != '\n')
		{
			FILE *fp = fopen(LAST_PGN_DIR, "w");
			if(!fp)
				return;
			fputs(pgn, fp);
			fclose(fp);
		}

		uint8_t pos[game->pos_size];
		memcpy(pos, game->initial_pos, game->pos_size);
		bool valid = load_pgn(game, pos, pgn);
		if(!valid)
			return;





		init_play_windows();

		if(DEV_MODE)
			play(game, pos, COMPUTER_PLAYER, HUMAN_PLAYER);
			//play(game, pos, HUMAN_PLAYER, COMPUTER_PLAYER);
		else
		{
			if(rand() & 0b1)
				play(game, NULL, HUMAN_PLAYER, COMPUTER_PLAYER);
			else
				play(game, NULL, COMPUTER_PLAYER, HUMAN_PLAYER);
		}

		printf("clearing game memory, wait a sec...\n");
		solver_clear();

		printf("play again? y/n\n");
		char opt = getchar();
		if(opt != 'y')
			break;


	}
}

void play(solver_t *solver, void *start_pos, bool p1, bool p2)
{
	solver_init(solver);



	int move;

	printf("starting %s! %s goes first.\n\n",
		solver->name, (p1==HUMAN_PLAYER)? "human":"computer");

	uint8_t pos[solver->pos_size];
	if(!start_pos)
		start_pos = solver->initial_pos;
	memcpy(pos, start_pos, solver->pos_size);
	//if(!pos)
	//	pos = solver->initial_pos;

	bool turn = !(seq_ct & 0b1);
	window_term_clear();
	clocks_init(5*60, 5*60);

	while(1)
	{
		term_move_cursor(0, 12);
		solver->draw_full(pos, seq_ct? seq[seq_ct-1] : -1);
		print_sequence_fancy(solver, seq, stdout);


		//for(int i=0; i<solver->possible_moves; i++)
		//	printf("move %d has placement %d\n", i, solver->get_placement(pos, i));

		window_focus(analysis_hdl);

		//term_move_cursor(0, 35);
		//printf(TERM_RESET);


		bool player = turn? p1 : p2;
		clock_resume(player);
		if(player == HUMAN_PLAYER)
		{
			/*
			draw position
			prompt for move, wait until given
			make move, update pos
			*/
			//solver->draw_full(pos);



			print_sequence(solver, seq, stdout);
			printf("\n\nenter your move, [b]ack, [f]orward, [s]ave, [q]uit game, or e[x]it:\n> ");
			char buf[160];
			char *bp = buf;
			bool end = false;
			while(1)
			{
				bool clock_flag = clock_update();
				if(clock_flag)
				{
					printf("human lost on time\n");
					return;
				}

				if(_kbhit())
				{
					char ch = _getch();
					switch(ch)
					{
						case '\n':
						case '\r':
							*bp = '\0';
							end = true;
							break;
						case '\b':
							if(bp > buf)
							{
								bp--;
								printf("\b \b");
							}
							break;
						default:
							if(bp < buf+159)
							{
								*bp++ = ch;
								printf("%c", ch);
							}
					}

				}

				if(end)
					break;
			}
			/*fgets(buf, 159, stdin);
			char *end = &buf[strlen(buf)-1];
			if(*end == '\n')
				*end = '\0';*/

			if(strcmp(buf, "b")==0 || strcmp(buf, "back")==0)
			{
				seq_ct -= 2;
				//pos = solver->initial_pos;
				memcpy(pos, solver->initial_pos, solver->pos_size);
				make_sequence_moves(solver, pos);
				continue;
			}
			else if(strcmp(buf, "f")==0 || strcmp(buf, "forward")==0)
			{
				if(seq_ct+2 <= seq_entire)
				{
					seq_ct += 2;
					//pos = solver->initial_pos;
					memcpy(pos, solver->initial_pos, solver->pos_size);
					make_sequence_moves(solver, pos);
				}
				continue;
			}
			else if(strcmp(buf, "s")==0 || strcmp(buf, "save")==0)
			{
				printf("enter save path (or enter for repeat):  \n");
				fgets(buf, 159, stdin);
				if(buf[0] == '\n')
					strncpy(buf, "repeat.txt", 159);
				else
				{
					char *end = &buf[strlen(buf)-1];
					if(*end == '\n')
						*end = '\0';
				}

				save_pgn(solver, buf);
				continue;
			}
			else if(strcmp(buf, "q")==0 || strcmp(buf, "quit game")==0)
			{
				return;
			}
			else if(strcmp(buf, "x")==0 || strcmp(buf, "exit")==0)
			{
				printf("ggs\n");
				exit(0);
			}



			#ifndef FORCE_MOVE_ITERATION
			if(solver->human_to_iter)
				move = solver->human_to_iter(buf);
			else
			#endif
				move = strtol(buf, NULL, 10);
			if(move < 0 || move >= solver->possible_moves
				|| !solver->is_legal(pos, move))
			{
				printf("illegal move!\n");
				continue;
			}
			solver->make_move(pos, move, NULL);


			/*printf("enter to continue   ");
			getchar();*/
		}
		else	//COMPUTER_PLAYER
		{
			#ifdef FORCE_SEARCH_DEPTH
			int time_lim = 0;
			#else
			//int time_lim = COMP_TIME;
			int time_lim = clock_get_time() / ((42-seq_ct)/2);
			#endif
			move = solve(solver, pos, seq_ct, time_lim, true);
			solver->make_move(pos, move, NULL);
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
			term_move_cursor(0, 12);
			solver->draw_full(pos, -1);
			print_sequence(solver, seq, stdout);
			break;
		}

		turn = !turn;

		seq_add(move);
	}
}

bool load_pgn(solver_t *solver, void *pos, char *in)
{
	//
	//reset
	seq_ct = 0;
	seq_entire = 0;

	//void *pos = solver->initial_pos;
	if(in[strlen(in)-1] == '\n')
		in[strlen(in)-1] = '\0';
	if(strstr(in, ".txt"))
	{
		if(!read_pgn_file(solver, in))
			return false;
	}
	else
		load_seq_string(solver, in);
	return make_sequence_moves(solver, pos);
}

bool read_pgn_file(solver_t *solver, char *name)
{
	//make file name
	char buf[640];
	snprintf(buf, 639, "%s/%s", PGN_DIR, name);

	//open file
	FILE *fp = fopen(buf, "r");
	if(!fp)
	{
		printf("invalid file %s\n", buf);
		return false;
	}

	//load moves
	fgets(buf, 639, fp);
	//return load_seq_string(solver, pos, buf);
	load_seq_string(solver, buf);
	//return make_sequence_moves(solver, pos);
	fclose(fp);
	return true;
}

void save_pgn(solver_t *solver, char *path)
{
	//make file name
	char buf[640];
	snprintf(buf, 639, "%s/%s", PGN_DIR, path);

	//open file
	FILE *fp = fopen(buf, "w");
	if(!fp)
	{
		printf("invalid file %s\n", buf);
		return;
	}

	//write moves
	print_sequence(solver, seq, fp);

	fclose(fp);
}

void load_seq_string(solver_t *solver, char *seq)
{
	for(char *token = strtok(seq, ","); token;
		token = strtok(NULL, ","))
	{
		if(*token == '\n')
			return;

		while(*token==' ' || *token=='\t')
			token++;

		int move = -1;
		if(solver->human_to_iter)
			move = solver->human_to_iter(token);
		if(move < 0 || move >= solver->possible_moves)
			move = strtol(token, NULL, 10);
		printf("move=%d\n", move);

		seq_add(move);

		/*if(solver->is_legal(pos, move))
		{
			solver->make_move(pos, move, NULL);
			seq_add(move);
		}
		else
		{
			printf("illegal move %d\n", move);
			return false;
		}*/
	}

	//return true;
}

bool make_sequence_moves(solver_t *solver, void *pos)
{
	for(int i=0; i<seq_ct; i++)
	{
		int move = seq[i];
		if(solver->is_legal(pos, move))
		{
			solver->make_move(pos, move, NULL);
		}
		else
		{
			printf("illegal move %d\n", move);
			return false;
		}
	}

	return true;
}

void print_sequence(solver_t *solver, int8_t *seq, FILE *stream)
{
	bool to_term = (stream == stdout);
	bool partial = (seq_entire != seq_ct);

	if(to_term)
		printf("\n\nmoves played: ");
	//for(int8_t *move = seq; *move!=-1; move++)
	for(int i=0; i<seq_entire; i++)
	{
		if(to_term && partial && i==seq_ct)
			printf("(");
		//printf("%d, ", *move);

		if(to_term)
		{
			if(solver->iter_to_human)
				printf("%s", solver->iter_to_human(seq[i]));
			else
				printf("%d", seq[i]);
		}
		else
		{
			if(solver->iter_to_human)
				fprintf(stream, "%s", solver->iter_to_human(seq[i]));
			else
				fprintf(stream, "%d", seq[i]);
		}

		if(i==seq_entire-1)
		{
			if(to_term && partial)
				printf(")");
		}
		else
			fprintf(stream, ", ");

	}

	if(to_term)
		printf("\n");
}

void print_sequence_fancy(solver_t *solver, int8_t *seq, FILE *stream)
{
	window_focus(notation_win_hdl);
	window_clear();

	bool to_term = (stream == stdout);
	bool partial = (seq_entire != seq_ct);

	//for(int8_t *move = seq; *move!=-1; move++)
	for(int i=0; i<seq_entire; i++)
	{
		if(to_term && partial && i==seq_ct)
			printf("(");

		int chars;
		if(!(i&0b1))
		{
			if(i)	printf("\n");
			chars = printf("%d.", i/2+1);
			for(int n=0; n<5-chars; n++)
				printf(" ");
		}
		else
		{
			printf("|   ");
		}

		if(solver->iter_to_human)
			chars = printf("%s", solver->iter_to_human(seq[i]));
		else
			chars = printf("%d", seq[i]);
		for(int n=0; n<4-chars; n++)
			printf(" ");

		if(i==seq_entire-1)
		{
			if(to_term && partial)
				printf(")");
		}

	}

	if(to_term)
		printf("\n");
}

void seq_add(int move)
{
	//if we're modifying past history, erase the rest of it
	if(seq_ct != seq_entire && seq[seq_ct] != move)
		seq_entire = seq_ct;

	seq[seq_ct] = move;
	seq_ct++;
	seq[seq_ct] = -1;
	if(seq_ct > seq_entire)
		seq_entire = seq_ct;
}
