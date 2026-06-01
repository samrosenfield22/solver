

#include "windowing.h"
#include "winterm.h"
//#include "../utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <signal.h>

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
char NO_BORDER_STYLE[6] = {0, 0, 0, 0, 0, 0};


//
text_window_t *window_from_hdl(int hdl);
void window_draw(text_window_t *win);
void window_home(text_window_t *win);
void window_add_char(text_window_t *win, char c);
void window_fit(text_window_t *win);
void window_newline(text_window_t *win);
void window_sys_init(void);


#define MAX_WINDOWS		(16)
text_window_t ALL_WINDOWS[MAX_WINDOWS];
int WIN_CT = 0;

text_window_t *CURRENT_WINDOW = NULL;
bool WINDOW_INITIALIZED = false;


int window(int x, int y)
{
	return window_wh(x, y, DEFAULT_WIN_W, DEFAULT_WIN_H);
}

int window_wh(int x, int y, int w, int h)
{
	if(!WINDOW_INITIALIZED)
	{
		window_sys_init();
	}

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

	CURRENT_WINDOW = win;

	int hdl = WIN_CT;
	WIN_CT++;
	return hdl;
}

void window_resize(int w, int h)
{
	text_window_t *win = CURRENT_WINDOW;

	win->w = w;
	win->h = h;

	//window_fit

	window_draw(win);
}

void window_set_colors(char *fg, char *bg)
{
	text_window_t *win = CURRENT_WINDOW;

	win->fg = fg;
	win->bg = bg;

	window_draw(win);
}


int window_printf(const char *fmt, ...)
{
	//passthrough
	if(!CURRENT_WINDOW)
	{
		va_list args;
		va_start(args, fmt);
		int n = vprintf(fmt, args);
		va_end(args);
		return n;
	}


	text_window_t *win = CURRENT_WINDOW;

	assert(win->cursor_x >= 0);
	assert(win->cursor_y >= 0);

	term_move_cursor(win->x+win->cursor_x, win->y+win->cursor_y);
	printf("%s%s", win->fg, win->bg);

	//print everything to a temp buf first
	char buf[321];
	va_list args;
	va_start(args, fmt);
	int n = vsnprintf(buf, 320, fmt, args);
	va_end(args);

	for(char *p=buf; *p; p++)
	{
		window_add_char(win, *p);
	}

	printf(TERM_RESET);
	return n;
}

void window_clear(void)
{
	text_window_t *win = CURRENT_WINDOW;

	win->cursor_x = 0;
	win->cursor_y = 0;
	win->wp = win->buf;
	*win->wp = '\0';

	window_draw(win);
}

void window_term_clear(void)
{
	term_clear();
	for(int i=0; i<WIN_CT; i++)
	{
		text_window_t *win = &ALL_WINDOWS[i];
		window_draw(win);
	}
}

void window_cursor_set(int y)
{
	text_window_t *win = CURRENT_WINDOW;
	window_home(win);

	for(int i=0; i<y; i++)
	{
		for(int x=0; x<win->w; x++)
		{
			if(*win->wp == '\n')
			{
				//x = win->w;
				win->wp++;
				break;
			}
			else if(*win->wp == '\0')
			{
				/*int addl_lines = y - i;
				for(int a=0; a<addl_lines; a++)
					*win->wp++ = '\n';
				*win->wp = '\0';
				win->wp -= addl_lines;
				break;*/
				return;
			}


			win->wp++;
			win->cursor_x++;
		}

		win->cursor_x = 0;
		win->cursor_y++;
	}
}

void window_focus(int hdl)
{
	text_window_t *win = window_from_hdl(hdl);

	//printf("focusing on window %d (cursor to %d,%d)\n",
	//	hdl, win->x+win->cursor_x, win->y+win->cursor_y);

	term_move_cursor(win->x+win->cursor_x, win->y+win->cursor_y);
	printf("%s%s", win->fg, win->bg);
	CURRENT_WINDOW = win;
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
				printf(TERM_RESET);
				return;
			}
			else
				putchar(*rp++);
		}
	}

	//win->cursor_y = 0;
	printf(TERM_RESET);
}

void window_home(text_window_t *win)
{
	win->cursor_x = 0;
	win->cursor_y = 0;
	win->wp = win->buf;
	//window_focus(win);
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


void window_newline(text_window_t *win)
{
	//printf("%s", TERM_RESET);
	win->cursor_x = 0;
	win->cursor_y++;
	term_move_cursor(win->x+win->cursor_x, win->y+win->cursor_y);
	//printf("%s%s", win->fg, win->bg);
}

void on_exit(void)
{
	printf(TERM_RESET);
	term_bottom();
}

void on_sigint(int n)
{
	(void)n;
	on_exit();
	exit(0);
}

void window_sys_init(void)
{
	winterm_init_ansi();

	atexit(on_exit);
	signal(SIGINT, on_sigint);

	WINDOW_INITIALIZED = true;
}
