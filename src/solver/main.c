

#include <stdio.h>
#include <string.h>

#include "../utils/utils.h"

#include "solver.h"
#include "play.h"

#include "games/nim.h"
#include "games/ttt.h"
#include "games/c4.h"

#include <windows.h>



int main(int argc, char **argv)
{
	if(argc >= 2)
	{
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
