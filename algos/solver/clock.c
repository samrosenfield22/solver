

#include "clock.h"
#include "play_windows.h"
#include "../../utils.h"

#include <stdio.h>

#define printf(fmt, ...)	window_printf(fmt, ##__VA_ARGS__)

/*#define CLOCK_X		(58)
#define CLOCK_Y		(40)
#define CLOCK_SEP	(25)*/

void clock_draw(int *c);
void clock_draw_frame(void);

int CLOCKS[2];
int CLOCK_START;
//bool CURRENT = false;
int *CUR_CLOCK = &CLOCKS[0];

void clocks_init(int p1, int p2)
{
	CLOCKS[0] = 1000*p1;
	CLOCKS[1] = 1000*p2;

	clock_draw_frame();
	clock_draw(&CLOCKS[0]);
	clock_draw(&CLOCKS[1]);

}

int clock_get_time(void)
{
	return *CUR_CLOCK;
}

void clock_resume(bool who)
{
	//CURRENT = who;
	CUR_CLOCK = &CLOCKS[who];
	CLOCK_START = *CUR_CLOCK;
	tic();
}

//returns true if flags
bool clock_update(void)
{
	int clock_now = CLOCK_START - toc_ms();
	int clock_now_sec = clock_now / 1000;
	if(clock_now_sec != (*CUR_CLOCK)/1000)
	{
		*CUR_CLOCK = clock_now;
		clock_draw(CUR_CLOCK);
	}
	*CUR_CLOCK = clock_now;

	return (*CUR_CLOCK < 0);
}

bool clock_flag(void)
{
	return (*CUR_CLOCK < 0);
}

/////////////////////////////

void clock_draw(int *c)
{
	int sec = (*c)/1000;
	bool player = (c == &CLOCKS[1]);
	//window_unfocus();
	window_focus(player? p2_window_hdl : p1_window_hdl);
	window_cursor_set(2);
	printf("  %02d:%02d", sec/60, sec%60);
	//term_move_cursor(CLOCK_X+1 + CLOCK_SEP*player, CLOCK_Y);
	//printf("%s%02d:%02d%s",
	//	TERM_YELLOW, sec/60, sec%60, TERM_WHITE);

	window_focus(analysis_hdl);
}

void clock_draw_frame(void)
{
	window_focus(p1_window_hdl);
	printf("player 1\n\n\n\n");
	window_focus(p2_window_hdl);
	printf("player 2\n\n\n\n");
	/*for(int i=0; i<2; i++)
	{
		term_move_cursor(CLOCK_X + CLOCK_SEP*i, CLOCK_Y-2);
		printf("%splayer %d%s",
			TERM_YELLOW, i+1, TERM_WHITE);
	}*/

}
