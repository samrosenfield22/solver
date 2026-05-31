

#include "windowing.h"

#include "../utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
	int x, y, w, h;
	char *fg, *bg;
	int cursor_x, cursor_y;
	char *buf;
	char *wp;
	char *border_style;
} text_window_t;



char DEFAULT_BORDER_STYLE[6] = {205, 186, 201, 187, 200, 188};

//
text_window_t *window_from_hdl(int hdl);
void window_focus(text_window_t *win);
void window_draw(text_window_t *win);
void window_home(text_window_t *win);
void window_add_char(text_window_t *win, char c);
void window_fit(text_window_t *win);
void window_newline(text_window_t *win);



#define MAX_WINDOWS		(16)
text_window_t ALL_WINDOWS[MAX_WINDOWS];
int WIN_CT = 0;

int window(int x, int y)
{
	return window_wh(x, y, DEFAULT_WIN_W, DEFAULT_WIN_H);
}

int window_wh(int x, int y, int w, int h)
{
	if(WIN_CT >= MAX_WINDOWS)
		return -1;

	text_window_t *win = &ALL_WINDOWS[WIN_CT];
	win->x = x;
	win->y = y;
	win->w = w;
	win->h = h;
	win->fg = TERM_GREEN;
	win->bg = TERM_BLACK_BG;
	win->cursor_x = 0;
	win->cursor_y = 0;
	win->border_style = DEFAULT_BORDER_STYLE;
	win->buf = malloc(win->w * win->h);
	win->wp = win->buf;
	*win->wp = '\0';

	window_draw(win);

	int hdl = WIN_CT;
	WIN_CT++;
	return hdl;
}

void window_resize(int hdl, int w, int h)
{
	text_window_t *win = window_from_hdl(hdl);

	win->w = w;
	win->h = h;

	//window_fit

	window_draw(win);
}

void window_set_colors(int hdl, char *fg, char *bg)
{
	text_window_t *win = window_from_hdl(hdl);

	win->fg = fg;
	win->bg = bg;

	window_draw(win);
}


void window_printf(int hdl, const char *fmt, ...)
{
	text_window_t *win = window_from_hdl(hdl);

	term_move_cursor(win->x+win->cursor_x, win->y+win->cursor_y);
	printf("%s%s", win->fg, win->bg);

	//print everything to a temp buf first
	char buf[321];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, 320, fmt, args);
	va_end(args);

	for(char *p=buf; *p; p++)
	{
		window_add_char(win, *p);
	}

	printf(TERM_CLEAR);
}



//////////////////////////////////////////////////////

text_window_t *window_from_hdl(int hdl)
{
	if(hdl >= WIN_CT)
	{
		printf("error! invalid window handle %d\n", hdl);
		exit(0);
	}
	return &ALL_WINDOWS[hdl];
}

void window_draw(text_window_t *win)
{
	//window_home(win);
	//term_move_cursor(win->x-2, win->y-2);
	printf("%s%s", win->fg, win->bg);


	//draw empty window
	//window_focus(win);
	for(int y=-2; y<win->h+2; y++)
	{
		term_move_cursor(win->x-2, win->y+y);

		//top/bottom borders
		if(y == -2)
		{
			putchar(win->border_style[2]);	//nw
			for(int x=-1; x<win->w+1; x++)
				putchar(win->border_style[0]);	//n/s
			putchar(win->border_style[3]);	//ne
			continue;
		}
		else if(y == win->h+1)
		{
			putchar(win->border_style[4]);	//sw
			for(int x=-1; x<win->w+1; x++)
				putchar(win->border_style[0]);	//n/s
			putchar(win->border_style[5]);	//se
			continue;
		}

		for(int x=-2; x<win->w+2; x++)
		{
			if(x == -2 || x == win->w+1)
				putchar(win->border_style[1]);
			else
				putchar(' ');
		}

	}

	//print text
	char *rp = win->buf;
	for(int y=0; y<win->h; y++)
	{
		term_move_cursor(win->x, win->y+y);
		for(int x=0; x<win->w; x++)
		{
			if(*rp=='\n')
			{
				rp++;
				break;
			}
			else if(*rp=='\0')
			{
				printf(TERM_CLEAR);
				return;
			}
			else
				putchar(*rp++);
		}
	}

	//win->cursor_y = 0;
	printf(TERM_CLEAR);
}

void window_home(text_window_t *win)
{
	win->cursor_x = 0;
	win->cursor_y = 0;
	//rp = 0?
	window_focus(win);
}

void window_focus(text_window_t *win)
{
	term_move_cursor(win->x+win->cursor_x, win->y+win->cursor_y);
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

	*win->wp++ = c;
	*win->wp = '\0';

	window_fit(win);
}

void window_fit(text_window_t *win)
{
	//window_cursor_end(win);
	if(win->cursor_y < win->h)
		return;	//already fits

	int to_delete = win->w;
	for(int i=0; i<win->w; i++)
	{
		if(win->buf[i] == '\n')
		{
			to_delete = i+1;
			//to_delete = i;
			break;
		}
	}
	win->wp -= to_delete;
	memmove(win->buf, win->buf+to_delete, win->wp - win->buf);
	*win->wp = '\0';
	win->cursor_x = 0;
	win->cursor_y = win->h-1;

	window_draw(win);
}

void window_cursor_end(text_window_t *win)
{
	//win->cursor_x =
}

void window_newline(text_window_t *win)
{
	printf("%s", TERM_CLEAR);
	win->cursor_x = 0;
	win->cursor_y++;
	term_move_cursor(win->x+win->cursor_x, win->y+win->cursor_y);
	printf("%s%s", win->fg, win->bg);
}
