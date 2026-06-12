

#ifndef MENU_H_
#define MENU_H_

//row specifier for building non-grid based menus
/*typedef struct
{
	int len;
	int locs[];
} rowspec_t;
*/

typedef struct
{
	int x, y;	//relative to menu x/y
	//char *icon;
} menu_opt_t;

typedef struct
{
	int x, y;
	int rows, columns;
	menu_opt_t *options;
	char *cursor;

	int select_x, select_y;

	/*
	callback?
	bool wrap_x, wrap_y;
	*/
} menu_t;

//menu creation
menu_t *menu_custom(int x, int y, int r, int c,
	menu_opt_t *options, char *cursor);
menu_t *menu_grid(int x, int y, int r, int c,
	int r_space, int c_space, char *cursor);
menu_opt_t *menu_make_options(int *locs, int r, int c);

//menu operations
void menu_left(menu_t *m);
void menu_right(menu_t *m);

//menu access
int menu_get(menu_t *m);

#endif	//MENU_H_
