

#include "winterm.h"
#include <stdio.h>
#include <windows.h>

// Manually define the flag if the current SDK lacks it
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

int init = 0;

//HANDLE hConsole = NULL;

//int tfg = TERM_NEUTRAL, tbg = TERM_BLACK;

//void term_set_fgbg(void);

/*void term_fg(int fg)
{
	winterm_init_ansi();
	tfg = fg;
	//term_set_fgbg();
	char buf[7];
	snprintf(buf, 6, "\033[%dm", tfg+10);
	printf("%s", buf);
	snprintf(buf, 6, "\033[%dm", 30);
	printf("%s", buf);
}

void term_bg(int bg)
{
	tbg = bg;
	term_set_fgbg();
}

void term_clear(void)
{
	tfg = TERM_NEUTRAL;
	tbg = TERM_BLACK;
	//term_set_fgbg();
}*/

////////////////////////////////////

/*void term_set_fgbg(void)
{
	winterm_init_ansi();
		//hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	//int fgbg = tfg | (tbg<<4);
	//SetConsoleTextAttribute(hConsole, fgbg);
	char buf[7];
	snprintf(buf, 7, "\033[%dm", tfg);
	printf("%s", buf);
	//snprintf(buf, 6, "\033[%dm", tbg+10);
	//printf("%s", buf);
}*/

void term_move_cursor(int x, int y)
{
	winterm_init_ansi();
	char buf[15];
	snprintf(buf, 15, "\033[%d;%dH", y, x);
	printf(buf);
}

void term_bottom(void)
{
	term_move_cursor(0, 120);
}

void term_clear(void)
{
	winterm_init_ansi();
	printf("\033[H");	//home
	printf("\033[2J");	//clear

}

void winterm_init_ansi(void)
{
	if(init)
		return;


	// Get the standard output handle
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole == INVALID_HANDLE_VALUE) return;

    // Retrieve current console mode
    DWORD dwMode = 0;
    if (!GetConsoleMode(hConsole, &dwMode)) return;

    // Enable the virtual terminal processing flag
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hConsole, dwMode);

	init = 1;
}
