

#include <stdio.h>
#include <string.h>

#include "utils.h"

#include "algos/solver/solver.h"
#include "algos/solver/play.h"

#include "algos/solver/games/nim.h"
#include "algos/solver/games/ttt.h"
#include "algos/solver/games/c4.h"

#include <windows.h>



int main(void)
{
	play_menu();
	if(mem_check())
		printf("\nmemory good!\n");
	else
		printf("\nmemory error! still have %u memory units not freed!\n", (uint32_t)mem_count());
	return 0;
}
