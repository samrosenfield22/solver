

#ifndef WINDOWING_H_
#define WINDOWING_H_

#include <stdarg.h>

#define DEFAULT_WIN_W	(32)
#define DEFAULT_WIN_H	(16)

int window(int x, int y);
int window_wh(int x, int y, int w, int h);
void window_resize(int hdl, int w, int h);
void window_set_colors(int hdl, char *fg, char *bg);
void window_printf(int hdl, const char *fmt, ...);

#endif	//WINDOWING_H_
