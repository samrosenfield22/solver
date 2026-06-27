

#include "play.h"
#include "ui/play_windows.h"
#include "../utils/utils.h"
#include "clock.h"

#include "games/game_includes.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>


#define PGN_DIR			"src/solver/pgns"
#define LAST_PGN_DIR	"src/solver/pgns/repeat.txt"


//#define COMP_TIME	(1 * 1000)
#define TIME_ODDS	(15)

#define DEV_MODE	(true)
//#define DEV_MODE	(false)

void print_sequence(solver_t *solver, int8_t *seq, FILE *stream);
void print_sequence_fancy(solver_t *solver, int8_t *seq, FILE *stream);
void save_pgn(solver_t *solver, char *path);
bool read_pgn_file(solver_t *solver, char *name);
void load_seq_string(solver_t *solver, char *seq);
bool make_sequence_moves(solver_t *solver, void *pos);
void seq_add(int move);

int8_t seq[200] = {-1};
int seq_ct = 0, seq_entire = 0;
char *CMDLINE_ARG_PGN = NULL;

#define printf(fmt, ...)	window_printf(fmt, ##__VA_ARGS__)


void play_menu(void)
{
	srand(time(NULL));

	solver_t gamelist[] =
	{
		#include "games/gamelist.txt"
	};
	//bool game_selected = false;
	solver_t *game = NULL;
	term_clear();
	term_move_cursor(50, 20);
	printf("select a game!");

	int len = sizeof(gamelist)/sizeof(gamelist[0]);
	//int gamechoice;
	for(int i=0; i<len; i++)
	{
		term_move_cursor(50, 22+i);
		printf("%s\n", gamelist[i].name);
	}
	menu_t *m = menu_vert(48, 22, len, 1);
	menu_set(m, 2);	//default to c4
	int sel = menu_control_loop(m);
	game = &gamelist[sel];

	uint8_t pos[game->pos_size];
	memcpy(pos, game->initial_pos, game->pos_size);
	char pgn[640];
	if(CMDLINE_ARG_PGN)
	{
		if(strcmp(CMDLINE_ARG_PGN, "r")==0)	//pgn repeat
		{
			FILE *fp = fopen(LAST_PGN_DIR, "r");
			if(!fp)
				return;
			fgets(pgn, 639, fp);
			//printf("pgn repeat: %s\n", pgn);
			//exit(0);
			fclose(fp);
		}
		else
		{
			strncpy(pgn, CMDLINE_ARG_PGN, 639);
			FILE *fp = fopen(LAST_PGN_DIR, "w");
			if(!fp)
				return;
			fputs(CMDLINE_ARG_PGN, fp);
			fclose(fp);
		}

		bool valid = load_pgn(game, pos, pgn);
		if(!valid)
		{
			printf("invalid pgn\n");
			exit(0);
		}
	}

	while(1)
	{
		//printf("\n\n\n\n\nenter pgn, (r) for repeat, or press enter for a new game:\n");
		//char pgn[640];
		//fgets(pgn, 639, stdin);

		init_play_windows();

		//choose who goes first
		bool p1=COMPUTER_PLAYER, p2=HUMAN_PLAYER;
		if(!DEV_MODE && (rand() & 0b1))
		{
			p1 = HUMAN_PLAYER;
			p2 = COMPUTER_PLAYER;
		}

		game_outcome_t outcome = play(game, pos,
			COMPUTER_PLAYER, HUMAN_PLAYER);

		bool win_player;
		printf("game over!\n");
		switch(outcome.reason)
		{
			case GAME_UNFINISHED:
				break;
			case WIN_BY_CHECKMATE:
			case WIN_BY_TIMEOUT:
				win_player = (outcome.winner)? p1 : p2;
				printf("player %d (%s) wins by %s\n",
					outcome.winner,
					(win_player==HUMAN_PLAYER)? "human":"computer",
					(outcome.reason==WIN_BY_CHECKMATE)? "checkmate":"timeout");
				break;
			case DRAW_BY_STALEMATE:	printf("draw!\n");	break;
			default:	assert(0);
		}
		if(!outcome.reason==GAME_UNFINISHED)
		{
			window_focus(p1_window_hdl);
			window_cursor_set(4);
			printf("\n\n   %.1f", outcome.score);
			window_focus(p2_window_hdl);
			window_cursor_set(4);
			printf("\n\n   %.1f", 1-outcome.score);
		}
		window_unfocus();
		term_move_cursor(0, 12);
		game->draw_full(outcome.pos, -1);
		window_focus(analysis_hdl);
		print_sequence(game, seq, stdout);
		mem_free(outcome.pos);


		printf("clearing game memory, wait a sec...\n");
		solver_clear();

		printf("play again? y/n\n");
		char opt = getchar();
		if(opt != 'y')
			break;
		memcpy(pos, game->initial_pos, game->pos_size);

	}
}

game_outcome_t play(solver_t *solver, void *start_pos, bool p1, bool p2)
{
	solver_init(solver);
	int move;

	printf("starting %s! %s goes first.\n\n",
		solver->name, (p1==HUMAN_PLAYER)? "human":"computer");

	//uint8_t pos[solver->pos_size];
	void *pos = mem_malloc(solver->pos_size);
	if(!start_pos)
		start_pos = solver->initial_pos;
	memcpy(pos, start_pos, solver->pos_size);

	bool turn = !(seq_ct & 0b1);
	window_term_clear();
	menu_t *move_sel_menu = solver->menu_define();

	window_focus(p1_window_hdl);
	window_clear();
	window_focus(p2_window_hdl);
	window_clear();
	if(FORCE_SEARCH_DEPTH)
		clocks_init(0, 0);
	else
	{
		clocks_init(5*60*1000 / TIME_ODDS, 5*60*1000);

		int moves_est = 50;
		if(solver->moves_remaining)
			moves_est = solver->moves_remaining(pos)/2;
		int time_p_m = (1000 * 5*60 / TIME_ODDS) / moves_est;
		if(time_p_m <= 1000)
		{
			solver->iddfs_increment /= 2;
		}
		if(time_p_m <= 80)
		{
			solver->iddfs_increment /= 2;
		}
		if(!solver->iddfs_increment)
			solver->iddfs_increment = 2;
	}


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
			//printf("\n\nenter your move, [b]ack, [f]orward, [s]ave, [q]uit game, or e[x]it:\n> ");
			printf("\nyour move!\n");
			char buf[160];
			char *bp = buf;
			bool end = false;
			while(1)
			{
				if(!FORCE_SEARCH_DEPTH)
				{
					bool clock_flagged = clock_update();
					if(clock_flagged)
						return (game_outcome_t)
							{
								.score=1-player,
								.winner=!player,
								.reason=WIN_BY_TIMEOUT,
								.pos=pos
							};
				}


				int key = term_check_input();
				//int sel = menu_input_control(move_sel_menu, key);
				int sel = solver->menu_update(move_sel_menu, pos, key);
				if(sel != -1)
				{
					//*bp = '\0';
					//snprintf(buf, 159, "%d", sel);
					move = sel;
					end = true;
					printf("\n");
					break;
				}

				switch(key)
				{
					case 'x':
						exit(0);
						break;
					/*case '\n':
					case '\r':
						*bp = '\0';
						end = true;
						printf("\n");
						break;
					case '\b':
						if(bp > buf)
						{
							bp--;
							printf("\b \b");
						}
						break;*/


					default:
						//solver->menu_update(move_sel_menu, pos, ch);
						if(bp < buf+159)
						{
							*bp++ = key;
							printf("%c", key);
						}
				}

				if(end)
					break;
			}


			if(move < 0 || move >= solver->possible_moves
				|| !solver->is_legal(pos, move))
			{
				printf("illegal move!\n");
				continue;
			}
			solver->make_move(pos, move, NULL);
		}
		else	//COMPUTER_PLAYER
		{
			int time_lim;
			if(FORCE_SEARCH_DEPTH)
				time_lim = 0;
			else
			{
				//int time_lim = 1000;
				if(solver->moves_remaining)
				{
					int remaining = solver->moves_remaining(pos)/2;
					if(remaining)
						time_lim = clock_get_time() / remaining;
					else
						time_lim = clock_get_time();
				}
				else
					time_lim = clock_get_time() / 50;	//idk
			}
			window_focus(analysis_hdl);
			window_clear();
			move = solve(solver, pos, time_lim, true);
			solver->make_move(pos, move, NULL);
		}

		bool clock_flagged = clock_flag();
		if(clock_flagged)
			return (game_outcome_t)
				{
					.score=1-player,
					.winner=!player,
					.reason=WIN_BY_TIMEOUT,
					.pos=pos
				};

		endstate_t game_end_state = solver->gameover(pos);
		if(game_end_state != END_NOT_OVER)
		{
			int winscore = 0.5;
			int reason = DRAW_BY_STALEMATE;
			if(game_end_state != END_DRAW)
			{
				winscore = (game_end_state==END_P1_WON)? 1 : 0;
				reason = WIN_BY_CHECKMATE;
			}
			return (game_outcome_t)
				{
					.score=winscore,
					.winner=player,
					.reason=reason,
					.pos=pos
				};
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

	char *in_cpy = strdup(in);

	//void *pos = solver->initial_pos;
	if(in[strlen(in_cpy)-1] == '\n')
		in[strlen(in_cpy)-1] = '\0';
	if(strstr(in_cpy, ".txt"))
	{
		if(!read_pgn_file(solver, in_cpy))
			return false;
	}
	else
		load_seq_string(solver, in_cpy);

	free(in_cpy);
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
		//printf("move=%d\n", move);

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
