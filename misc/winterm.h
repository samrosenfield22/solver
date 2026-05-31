

#ifndef WINTERM_H_
#define WINTERM_H_

/*enum
{
	TERM_BLACK	= 0,
	TERM_BLUE	= 1,
	TERM_GREEN	= 2,
	TERM_CYAN	= 3,
	TERM_RED	= 4,
	TERM_PURPLE	= 5,
	TERM_YELLOW	= 6,
	TERM_NEUTRAL= 7,
	TERM_BRIGHT	= 8,
};*/

#define TERM_BLACK	"\033[30m"
#define TERM_RED	"\033[31m"
#define TERM_GREEN	"\033[32m"
#define TERM_YELLOW	"\033[33m"
#define TERM_BLUE	"\033[34m"
#define TERM_PURPLE	"\033[35m"
#define TERM_CYAN	"\033[36m"
#define TERM_WHITE	"\033[37m"

#define TERM_BLACK_BG	"\033[40m"
#define TERM_RED_BG		"\033[41m"
#define TERM_GREEN_BG	"\033[42m"
#define TERM_YELLOW_BG	"\033[43m"
#define TERM_BLUE_BG	"\033[44m"
#define TERM_PURPLE_BG	"\033[45m"
#define TERM_CYAN_BG	"\033[46m"
#define TERM_WHITE_BG	"\033[47m"

//#define TO_STR(x)	TO_STR_HELPER(x)
//#define TO_STR_HELPER(x)	#x
//#define TERM_FG(COL)	"\033[" #COL "m"
//#define TERM_BG(COL)	("\033[" #COL+10 "m")

//#define TERM_RED		("\033[0m")
#define TERM_CLEAR		("\033[0m")

/*enum
{
	TERM_BLACK	= 30,
	TERM_RED	= 31,
	TERM_GREEN	= 32,
	TERM_YELLOW	= 33,
	TERM_BLUE	= 34,
	TERM_PURPLE	= 35,
	TERM_CYAN	= 36,
	TERM_NEUTRAL	= 37,
};*/

/*void term_fg(int fg);
void term_bg(int bg);
void term_clear(void);*/
void winterm_init_ansi(void);
void term_move_cursor(int x, int y);
void term_clear(void);

#endif	//WINTERM_H_
