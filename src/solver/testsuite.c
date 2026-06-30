/*
needs to clear game draw
fix printing for long move sequences (leave room for ...)
solve() should return result so we can confirm pos score
test if pv is correct!
*/

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

#define INDENT_POS		(4)
#define INDENT_RESULT	(32)
#define TEST_WINDOW_X	(117)
#define TEST_WINDOW_W	(49)

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
		.moves="3,3,3,3,3,2,5,2,6,4,4,4,4,5,4,3,2,2,2,0,1,1",
		.expected={.score=9990, .best_move=0}
	},
	{
		.game="connect four",
		.moves="6,6,6,6,6,6,4,4,3,5,5,5,5,3,3,1,3,4",
		.expected={.score=9990, .best_move=3}
	},
	{
		.game="connect four",
		.moves="3,3,3,3,3,2,2,2,2,3,5,6,5,0,4,4,5,5",
		.expected={.score=9989, .best_move=4}
	},
	{
		.game="connect four",
		.moves="3,2,3,3,3,2,2,2,5,5,4,6,1,1",
		.expected={.score=9988, .best_move=3}
	},
	{
		.game="connect four",
		.moves="1,2,2,3,4,2,2,3,3,4,2,1",
		.expected={.score=9985, .best_move=0}
	},
	{
		.game="connect four",
		.moves="3,3,3,3,3,6,4,2,2,2",
		.expected={.score=9984, .best_move=2}
	},
	{
		.game="connect four",
		.moves="2,2,2,2,3,1,1,1,5,4",
		.expected={.score=0, .best_move=1}
	},
	{
		.game="connect four",
		.moves="4,3,4,4,3,3,3,6",
		.expected={.score=9984, .best_move=1}
	},
	{
		.game="connect four",
		.moves="3,3,3,3,3,5",
		.expected={.score=9982, .best_move=5}
	},

	{
		.game="tic-tac-toe",
		.moves="0,8,2,1",
		.expected={.score=9997, .best_move=6}
	},
	{
		.game="tic-tac-toe",
		.moves="4,1,8,0",
		.expected={.score=9997, .best_move=2}
	},

};

//
bool test_case(test_case_t *tcase);

int testwin_hdl;

void run_testsuite(void)
{
	testwin_hdl = window_wh(TEST_WINDOW_X, 3, TEST_WINDOW_W, 40);
	window_set_colors(TERM_WHITE, TERM_BLACK_BG);
	window_term_clear();
	int pass_ct = 0;
	int suite_len = sizeof(ALL_TEST_CASES)/sizeof(ALL_TEST_CASES[0]);
	printf("\tsolver test suite (%d test cases)\n\n", suite_len);
	int indent = printf("n");
	for(int i=indent; i<INDENT_POS; i++)
		printf(" ");
	indent = INDENT_POS;
	indent += printf("pos");
	for(int i=indent; i<INDENT_RESULT; i++)
		printf(" ");
	indent = INDENT_RESULT;
	printf("result\n");
	for(int i=0; i<TEST_WINDOW_W; i++)
		printf("-");

	for(int i=0; i<suite_len; i++)
	{
		indent = printf("%d", i);
		for(int i=indent; i<INDENT_POS; i++)
			printf(" ");
		indent = INDENT_POS;
		int gap = INDENT_RESULT-INDENT_POS-4;	//room for "... "
		char buf[gap];
		snprintf(buf, gap-1, ALL_TEST_CASES[i].moves);
		if(buf[gap-2] == ',')
			buf[gap-2] = '\0';
		indent += printf("%s", buf);
		if(gap < strlen(ALL_TEST_CASES[i].moves))
			indent += printf("... ");
		for(int i=indent; i<INDENT_RESULT; i++)
			printf(" ");
		bool pass = test_case(&ALL_TEST_CASES[i]);
		if(pass)
			pass_ct++;
	}

	printf("\n%d out of %d cases passed (%.1f%%)\n",
		pass_ct, suite_len, 100*(((float)pass_ct))/suite_len);
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

	//erase previous pos
	window_unfocus();
	term_move_cursor(0, 0);
	for(int y=0; y<35; y++)
	{
		for(int x=0; x<TEST_WINDOW_X-3; x++)
			putchar(' ');
		putchar('\n');
	}

	//draw
	//window_unfocus();
	term_move_cursor(15, 10);
	solver->draw_full(pos, -1);
	window_focus(testwin_hdl);

	//solve it
	FORCE_SEARCH_DEPTH = 100;
	solver_init(solver);
	//result_t result = solve(solver, pos, 0, false);
	//int calculated_move = result.best_move;
	int calculated_move = solve(solver, pos, 0, false);
	solver_clear();

	bool pass = true;
	if(calculated_move != tcase->expected.best_move)
		pass = false;
	//else if()
	if(pass)
		printf("%sPASS%s\n", TERM_GREEN, TERM_WHITE);
	else
		printf("%sFAIL%s\n\texpected move %d, got %d\n",
			TERM_RED, TERM_WHITE, tcase->expected.best_move, calculated_move);

	return pass;

	/*
	get game solver
	make init pos
	make test pos
	solve()
	*/
}
