

#include <stdio.h>
#include <string.h>

#include "../utils/utils.h"

#include "solver.h"
#include "play.h"

#include "games/nim.h"
#include "games/ttt.h"
#include "games/c4.h"

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
