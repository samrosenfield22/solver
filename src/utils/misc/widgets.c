

#include "widgets.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "winterm.h"



typedef struct
{
	int *x, *y;
	int len;
	int w, h;
	char point;
} graph_t;

void *graph_create(int *y, int len)
{
	graph_t *g = malloc(sizeof(*g));
	if(!g)
		return NULL;

	g->x = malloc(len * sizeof(int));
	if(!g->x)
	{
		free(g);
		return NULL;
	}
	//memcpy(g->x, )
	g->y = malloc(len * sizeof(int));
	if(!g->y)
	{
		free(g->x);
		free(g);
		return NULL;
	}
	memcpy(g->y, y, len * sizeof(int));
	g->len = len;
	g->w = len;
	g->h = 10;
	g->point = '*';

	return g;
}

void graph_set_dims(void *gg, int w, int h)
{
	graph_t *g = (graph_t *)gg;
	g->w = w;
	g->h = h;
}

void graph_draw(void *gg)
{
	graph_t *g = (graph_t *)gg;

	//get max, min
	int g_max = INT_MIN;
	int g_min = INT_MAX;
	for(int x=0; x<g->len; x++)
	{
		int d = g->y[x];
		if(d > g_max)
			g_max = d;
		if(d < g_min)
			g_min = d;
	}
	int range = g_max - g_min;
	int x_space = g->w / g->len;
	float y_space = (float)range / g->h;

	//draw points
	term_clear();
	for(int x=0; x<g->len; x++)
	{
		int x_draw = x * x_space + 10;
		int y_draw = (float)(g_max-g->y[x]) / y_space + 10;
		//printf("pt %d has h=%d\n", g->y[x], h);
		term_move_cursor(x_draw, y_draw);
		putchar(g->point);

		//interpolate
		if(x_space > 1 && x != g->len-1)
		{
			int dif = g->y[x+1] - g->y[x];
			int inter = dif / x_space;

			for(int xx=1; xx<=x_space; xx++)
			{
				int inter_y = g->y[x] + xx*inter;
				y_draw = (float)(g_max-inter_y) / y_space + 10;
				term_move_cursor(x_draw + xx, y_draw);
				putchar(g->point);
			}
		}

		if((x % 5)==0)
		{
			term_move_cursor(x_draw, 10+g->h+2);
			printf("%d", x);
		}
	}
}
