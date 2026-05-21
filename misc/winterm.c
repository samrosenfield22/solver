

#include "winterm.h"
#include <windows.h>

HANDLE hConsole = NULL;

int tfg = 7, tbg = 0;

void term_set_fgbg(void);

void term_fg(int fg)
{
	tfg = fg;
	term_set_fgbg();
}

void term_bg(int bg)
{
	tbg = bg;
	term_set_fgbg();
}

void term_clear(void)
{
	tfg = 7;
	tbg = 0;
	term_set_fgbg();
}

void term_set_fgbg(void)
{
	if(!hConsole)
		hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	int fgbg = tfg | (tbg<<4);
	SetConsoleTextAttribute(hConsole, fgbg);
}
