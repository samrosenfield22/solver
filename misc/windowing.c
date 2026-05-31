

#include "windowing.h"

#include "../utils.h"

#include <stdio.h>
#include <stdlib.h>

#define DEFAULT_WIN_W	(40)
#define DEFAULT_WIN_H	(20)

typedef struct
{
	int x, y, w, h;
	char *fg, *bg;
	int cursor_x, cursor_y;
	//border_style;
} text_window_t;

//
text_window_t *window_from_hdl(int hdl);
void window_focus(text_window_t *win);
void window_add_char(text_window_t *win, char c);
void window_newline(text_window_t *win);



#define MAX_WINDOWS		(16)
text_window_t ALL_WINDOWS[MAX_WINDOWS];
int WIN_CT = 0;

int window(int x, int y)
{
	text_window_t *win = &ALL_WINDOWS[WIN_CT];
	win->x = x;
	win->y = y;
	win->w = DEFAULT_WIN_W;
	win->h = DEFAULT_WIN_H;
	win->fg = TERM_WHITE;
	win->bg = TERM_BLUE_BG;
	win->cursor_x = 0;
	win->cursor_y = 0;
	//win->buf = malloc(win->w * win->h);

	//draw empty window
	window_focus(win);
	for(int yy=0; yy<win->h; yy++)
	{
		for(int xx=0; xx<win->w; xx++)
			putchar(' ');
		window_newline(win);
		//printf("y at %d\n", win->cursor_y);
	}
	printf("%s\ncursor at %d,%d\n", TERM_CLEAR, win->cursor_x, win->cursor_y);

	int hdl = WIN_CT;
	WIN_CT++;
	return hdl;
}

void window_set_dims(int hdl, int w, int h)
{
	text_window_t *win = window_from_hdl(hdl);

	win->w = w;
	win->h = h;
	//redraw
}

void window_set_colors(int hdl, char *fg, char *bg)
{
	text_window_t *win = window_from_hdl(hdl);

	win->fg = fg;
	win->bg = bg;
	//redraw
}



void window_printf(int hdl, const char *fmt, ...)
{
	text_window_t *win = window_from_hdl(hdl);

	move_cursor(win->x+win->cursor_x, win->y+win->cursor_y);
	printf("%s%s", win->fg, win->bg);

	//print everything to buf first
	char buf[321];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, 320, fmt, args);
	va_end(args);




	for(char *p=buf; *p; p++)
	{
		window_add_char(win, *p);
	}
}



///////////////////////

text_window_t *window_from_hdl(int hdl)
{
	if(hdl >= WIN_CT)
	{
		printf("error! invalid window handle %d\n", hdl);
		exit(0);
	}
	return &ALL_WINDOWS[hdl];
}

void window_focus(text_window_t *win)
{
	move_cursor(win->x+win->cursor_x, win->y+win->cursor_y);
	printf("%s%s", win->fg, win->bg);
}

void window_add_char(text_window_t *win, char c)
{
	if(c == '\n')
	{
		window_newline(win);
	}
	else
	{
		putchar(c);
		win->cursor_x++;
		if(win->cursor_x >= win->w)
		{
			window_newline(win);
		}
	}
}

void window_newline(text_window_t *win)
{
	printf("%s", TERM_CLEAR);
	win->cursor_x = 0;
	win->cursor_y++;
	move_cursor(win->x+win->cursor_x, win->y+win->cursor_y);
	printf("%s%s", win->fg, win->bg);
}
