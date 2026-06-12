

#include "menu.h"
#include "../utils.h"

#include <stdio.h>
#include <stdbool.h>

//statics
void menu_draw_cursor(menu_t *m, bool erase);
void menu_opt_xy(menu_t *m, int select, int *x, int *y);

menu_t *menu_custom(int x, int y, int r, int c,
	menu_opt_t *options, char *cursor)
{
	menu_t *m = mem_malloc(sizeof(*m));

	m->x = x;
	m->y = y;
	m->rows = r;
	m->columns = c;
	m->options = options;
	m->select_x = 0;
	m->select_y = 0;
	m->cursor = cursor;

	return m;
}

menu_t *menu_grid(int x, int y, int r, int c,
	int r_space, int c_space, char *cursor)
{
	int opt_len = r * c;
	/*menu_opt_t *options = malloc(opt_len * sizeof(*options));
	for(int i=0; i<opt_len; i++)
	{
		menu_opt_t *opt = &options[i];

		opt->x = (i%c) * r_space;
		opt->y = (i/c) * c_space;
		//opt->icon = icon;
	}*/
	int locs[2*opt_len];
	for(int i=0; i<opt_len; i++)
	{
		locs[2*i] = (i%c) * c_space;
		locs[2*i+1] = (i/c) * r_space;
	}

	menu_opt_t *options = menu_make_options(locs, r, c);
	/*printf("menu w %d options:\n", opt_len);
	for(int i=0; i<opt_len; i++)
	{
		printf("(%d,%d), ", options[i].x, options[i].y);
	}*/
	return menu_custom(x, y, r, c, options, cursor);
}

/*row points to an array of row arrays, i.e.
rowspec row_options[] =
{
	//row 0 w options at (0,0), (3,0), (6,1)
	{.locs={0, 0, 3, 0, 6, 1}, .len=3},

	//row 1 w options at (2,5), (5,5)
	{.locs={2, 5, 5, 5}, .len=2},
}
menu_make_options(row_options, sizeof());
*/
/*
actually no
locs is an array of alternating (x,y) pairs
*/
menu_opt_t *menu_make_options(int *locs, int r, int c)
{
	int opt_len = r*c;
	menu_opt_t *options = mem_malloc(opt_len * sizeof(*options));
	for(int i=0; i<opt_len; i++)
	{
		menu_opt_t *opt = &options[i];

		opt->x = locs[2*i];
		opt->y = locs[2*i+1];
	}

	return options;
}

void menu_left(menu_t *m)
{
	if(m->select_x <= 0)
		return;
	printf("going left!\n");
	menu_draw_cursor(m, true);	//erase
	m->select_x--;
	menu_draw_cursor(m, false);	//draw

	/*int cursor_x, cursor_y;
	menu_opt_xy(m, menu_get(m), &cursor_x, &cursor_y);
	printf("cursor at %d,%d\n", cursor_x, cursor_y);*/
}

void menu_right(menu_t *m)
{
	if(m->select_x >= m->columns-1)
		return;
	printf("going right!\n");
	menu_draw_cursor(m, true);	//erase
	m->select_x++;
	menu_draw_cursor(m, false);	//draw

	/*int cursor_x, cursor_y;
	menu_opt_xy(m, menu_get(m), &cursor_x, &cursor_y);
	printf("cursor at %d,%d\n", cursor_x, cursor_y);*/
}

int menu_get(menu_t *m)
{
	return m->columns * m->select_y + m->select_x;
}

//////////////////////////////////////////////////

void menu_draw_cursor(menu_t *m, bool erase)
{
	int cursor_x, cursor_y;
	menu_opt_xy(m, menu_get(m), &cursor_x, &cursor_y);

	//term_move_cursor(cursor_x, cursor_y);
	int x=0, y=0;
	bool jump = true;
	for(char *c=m->cursor; *c; c++)
	{
		if(jump)
			term_move_cursor(x+cursor_x, y+cursor_y);
		jump = true;

		if(*c == '\n')
		{
			x = 0;
			y++;
		}
		else if(*c == ' ')
			x++;
		else
		{
			printf("%c", erase? ' ' : *c);
			x++;
			jump = false;
		}
	}
}

void menu_opt_xy(menu_t *m, int select, int *x, int *y)
{
	*x = m->x + m->options[select].x;
	*y = m->y + m->options[select].y;
}
