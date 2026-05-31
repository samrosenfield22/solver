

#ifndef WINDOWING_H_
#define WINDOWING_H_

#include <stdarg.h>

int window(int x, int y);
void window_set_dims(int hdl, int w, int h);
void window_set_colors(int hdl, char *fg, char *bg);
void window_printf(int hdl, const char *fmt, ...);

#endif	//WINDOWING_H_
