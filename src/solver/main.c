

#include <stdio.h>
#include <string.h>

#include "../utils/utils.h"

#include "solver.h"
#include "play.h"
#include "testsuite.h"

/*#include "games/nim.h"
#include "games/ttt.h"
#include "games/c4.h"*/

#include <windows.h>



int main(int argc, char **argv)
{
	if(argc >= 2)
	{
		if(strcmp(argv[1], "testsuite")==0
			|| strcmp(argv[1], "test")==0)
		{
			run_testsuite();
			return 0;
		}
		FORCE_SEARCH_DEPTH = strtol(argv[1], NULL, 10);
	}
	if(argc == 3)
	{
		CMDLINE_ARG_PGN = argv[2];
	}


	play_menu();
	if(mem_check())
		printf("\nmemory good!\n");
	else
		printf("\nmemory error! still have %u memory units not freed!\n", (uint32_t)mem_count());
	return 0;
}
