

#include <stdio.h>
#include <string.h>

#include "utils.h"

#include "algos/solver/solver.h"
#include "algos/solver/play.h"

#include "algos/solver/games/nim.h"
#include "algos/solver/games/ttt.h"
#include "algos/solver/games/c4.h"

#include <windows.h>



int main(void)
{
	//int locs[] = {0, 0, 3, 0, 5, }
	//void *opt = menu_make_options()
	/*menu_t *m = menu_grid(20, 20,	//x,y
		1, 7,	//rows,columns
		1, 3,	//r/c spacing
		"\n^\n^");
	term_clear();
	while(1)
	{

		term_move_cursor(1,20);
		char c = getchar();
		if(c == '1')
			menu_left(m);
		else if(c == '2')
			menu_right(m);

		//printf("\n\nselect=%d", menu_get(m));
	}

	return 0;*/

	//winterm_init_ansi();

	/*term_clear();
	int mywin = window(20, 20);
	int other = window_wh(50, 15, 90, 10);
	window_printf(mywin, "test %d", 67);
	window_printf(other, "\n\naaaaaaaaaaaaaaaaaaaaaaaaayyyyyyyyyyyyyyy");
	window_printf(mywin, "\n\nmustard on the beat h%02d\n", 3);
	window_set_colors(other, TERM_CYAN, TERM_BLACK_BG);
	int i=0;
	while(1)
	{
		window_printf(mywin, "%d\n", i);
		i++;
		delay(500);
	}
	return 0;
	*/

	play_menu();
	if(mem_check())
		printf("\nmemory good!\n");
	else
		printf("\nmemory error! still have %u memory units not freed!\n", (uint32_t)mem_count());
	return 0;






	//printf("size of nim pos is %u bytes\n", sizeof(nim_pos_t));

	//play(&TTT_SOLVER, NULL, HUMAN_PLAYER, COMPUTER_PLAYER);
	play(&NIM_SOLVER, NULL, HUMAN_PLAYER, COMPUTER_PLAYER);
	return 0;

	//solve(&NIM_SOLVER, NULL);
	//return 0;
	//solve(&TTT_SOLVER, NULL);
	//return 0;



	//tree_demo();
	//hashmap_demo();
	//getchar();
	//printf("waiting 5 seconds...\n");
	//delay(5000);
	//printf("done!\n");
	return 0;

	/*
	list_t *ml = list(char *);
	list_append(ml, &(char*){"hello"});
	list_append(ml, &(char*){"theremyfriend"});
	printf("last is \'%s\'\n", *(char**)list_last(ml));
	///list_delete(ml);
	//printf("deleted!\n");
	//list_last(ml);
	//list_delete(ml);
	//printf("deleted!\n");
	list_append(ml, "other");
	//printf("appended!\n");
	list_print(ml);
	//printf("last is \'%s\'\n", *(char**)list_last(ml));
	list_iterate(ml, item)
	{
		printf("item %s is last? %s\n", *(char**)item,
			list_islast(ml)? "true":"false");
	}

	return 0;
	*/







	//

	/*int *v = vector(int, 12);
	for(int i=0; i<8; i++)
		v[i] = i;
	for(int i=0; i<8; i++)
		printf("%d\n", v[i]);

	vector_print_meta(v);
	vector_resize(&v, 15000);
	vector_print_meta(v);

	for(int i=0; i<8; i++)
		printf("%d\n", v[i]);

	vector_resize(&v, 10);*/

	return 0;
}
