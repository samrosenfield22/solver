

#include "testsuite.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "shared.h"
#include "play.h"
#include "../utils/misc/winterm.h"
#include "../utils/misc/windowing.h"
#include "games/game_includes.h"

#define printf(fmt, ...)	window_printf(fmt, ##__VA_ARGS__)

typedef struct
{
	char *game;
	char *moves;
	result_t expected;
} test_case_t;

test_case_t ALL_TEST_CASES[] =
{
	{
		.game="connect four",
		.moves="3,3,3,3,3,5",
		.expected={.score=9982, .best_move=5}
	},
};

//
bool test_case(test_case_t *tcase);

int testwin_hdl;

void run_testsuite(void)
{
	testwin_hdl = window_wh(117, 3, 49, 40);
	window_set_colors(TERM_WHITE, TERM_WHITE);
	window_term_clear();
	int pass_ct = 0;
	int suite_len = sizeof(ALL_TEST_CASES)/sizeof(ALL_TEST_CASES[0]);
	printf("RUNNING TEST SUITE\nn\tpos\t\tresult\n-------------------------------\n");
	for(int i=0; i<suite_len; i++)
	{
		printf("%d\t%s\t", i, ALL_TEST_CASES[i].moves);
		bool pass = test_case(&ALL_TEST_CASES[i]);
		if(pass)
			pass_ct++;
	}

	printf("\n%d out of %d cases passed (%.1f%%)\n",
		pass_ct, suite_len, (((float)pass_ct))/suite_len);
}

bool test_case(test_case_t *tcase)
{
	//get game solver
	solver_t gamelist[] =
	{
		#include "games/gamelist.txt"
	};
	solver_t *solver = NULL;
	for(int i=0; i<sizeof(gamelist)/sizeof(gamelist[0]); i++)
	{
		if(strcmp(gamelist[i].name, tcase->game)==0)
		{
			solver = &gamelist[i];
			break;
		}
	}
	if(!solver)
	{
		printf("invalid game \'%s\' in test case\n",
			tcase->game);
		exit(0);
	}

	//make test pos
	uint8_t pos[solver->pos_size];
	memcpy(pos, solver->initial_pos, solver->pos_size);
	if(!load_pgn(solver, pos, tcase->moves))
	{
		printf("invalid move sequence in test case:\n\t%s\n",
			tcase->moves);
		exit(0);
	}

	//draw
	window_unfocus();
	term_move_cursor(15, 10);
	solver->draw_full(pos, -1);
	window_focus(testwin_hdl);

	//solve it
	FORCE_SEARCH_DEPTH = 100;
	solver_init(solver);
	int calculated_move = solve(solver, pos, 0, false);
	solver_clear();

	bool pass = (calculated_move == tcase->expected.best_move);
	if(pass)
		printf("%sPASS%s\n", TERM_GREEN, TERM_WHITE);
	else
		printf("%sFAIL%s (expected move %d, got %d)\n",
			TERM_RED, TERM_WHITE, tcase->expected.best_move, calculated_move);

	return pass;

	/*
	get game solver
	make init pos
	make test pos
	solve()
	*/
}
