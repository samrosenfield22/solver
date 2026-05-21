

#ifndef WINTERM_H_
#define WINTERM_H_

enum
{
	TERM_BLUE	= 1,
	TERM_GREEN	= 2,
	TERM_CYAN	= 3,
	TERM_RED	= 4,
	TERM_PURPLE	= 5,
	TERM_YELLOW	= 6,
	TERM_NEUTRAL= 7,
	TERM_BRIGHT	= 8,
};

void term_fg(int fg);
void term_bg(int bg);
void term_clear(void);


#endif	//WINTERM_H_
