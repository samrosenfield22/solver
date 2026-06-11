

#include "clock.h"
#include "play_windows.h"
#include "../../utils.h"

#include <stdio.h>

void clock_draw(bool player);

int CLOCKS[2];
int CLOCK_START;
int *CUR_CLOCK;

void clocks_init(int p1, int p2)
{
	CLOCKS[0] = 1000*p1;
	CLOCKS[1] = 1000*p2;

	clock_draw(0);
	clock_draw(1);
}

void clock_resume(bool who)
{
	CUR_CLOCK = &CLOCKS[who];
	CLOCK_START = *CUR_CLOCK;
	tic();
}

bool clock_update(bool player)
{
	int clock_now = CLOCK_START - toc_ms();
	int clock_now_sec = clock_now / 1000;
	if(clock_now_sec != (*CUR_CLOCK)/1000)
	{
		*CUR_CLOCK = clock_now;
		clock_draw(player);
	}
	*CUR_CLOCK = clock_now;

	return (*CUR_CLOCK > 0);
}

/////////////////////////////

void clock_draw(bool player)
{
	int sec = CLOCKS[player]/1000;
	window_unfocus();
	term_move_cursor(32 + 10*player, 40);
	printf("%s%02d:%02d%s",
		TERM_YELLOW, sec/60, sec%60, TERM_WHITE);

	window_focus(analysis_hdl);
}
