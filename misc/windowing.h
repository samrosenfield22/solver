

#ifndef WINDOWING_H_
#define WINDOWING_H_

#include <stdarg.h>

#define DEFAULT_WIN_W	(32)
#define DEFAULT_WIN_H	(16)

enum
{
	DEFAULT_BORDER_STYLE,
	NO_BORDER_STYLE,
};

//#define printf(fmt, ...)	window_printf(fmt, ##__VA_ARGS__)

int window(int x, int y);
int window_wh(int x, int y, int w, int h);
void window_resize(int w, int h);
void window_set_colors(char *fg, char *bg);
void window_set_border(int style);
int window_printf(const char *fmt, ...);
void window_clear(void);
void window_term_clear(void);
void window_cursor_set(int y);
void window_focus(int hdl);
void window_unfocus(void);

#endif	//WINDOWING_H_
