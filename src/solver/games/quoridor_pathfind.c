

#include "../../utils/utils.h"
#include "quoridor_pathfind.h"

#include <assert.h>

//
void spread_wave(cell_t *n);
void propagate_dists(cell_t *n);
bool is_good(cell_t *n);
int get_neighbors(cell_t *map, cell_t *n, cell_t **nei);

cell_t *map = NULL;
__int128 horiz, vert;

#define CELL_Q_LEN	(300)

typedef struct
{
	cell_t *queue[CELL_Q_LEN];
	cell_t **next, **last;
} cell_queue_t;


void cell_queue_init(cell_queue_t *q)
{
	q->next = q->queue;
	q->last = q->queue;
}

bool cell_queue_isempty(cell_queue_t *q)
{
	assert(q->next <= q->last);
	return (q->next == q->last);
}

void cell_queue_enq(cell_queue_t *q, cell_t *n)
{
	//q->queue[q->last] = n;
	*(q->last) = n;
	q->last++;
	//assert(q->last < q->queue + CELL_Q_LEN);
}

cell_t *cell_queue_deq(cell_queue_t *q)
{
	if(cell_queue_isempty(q))
		return NULL;

	//cell_t *n = q->queue[q->next];
	cell_t *n = *(q->next);
	q->next++;
	return n;
}
//void cell_queue_(cell_queue_t *q)

void map_init(cell_t *m, bool whosemove)
{
	//bool whosemove = true;
	for(int i=0; i<81; i++)
	{
		m[i].dist = whosemove? 8-i/9: i/9;
		m[i].status = CELL_UNCHECKED;
	}
}

//list_t *wave_queue = NULL, *wave_ends_queue = NULL;
cell_queue_t wave_queue, wave_ends_queue;

void update_dists(cell_t *m, int *next_to_gate,
	__int128 h, __int128 v)
{
	map = m;
	horiz = h;
	vert = v;

	//init lists
	/*if(!wave_queue)
		wave_queue = list(cell_t *);
	if(!wave_ends_queue)
		wave_ends_queue = list(cell_t *);*/
	cell_queue_init(&wave_queue);
	cell_queue_init(&wave_ends_queue);

	//mark every cell as good
	for(int i=0; i<81; i++)
		map[i].status = CELL_UNCHECKED;

	//add all cells adjacent to the gate
	for(int i=0; i<4; i++)
	{
		int index = next_to_gate[i];
		cell_t *n = &map[index];
		n->status = CELL_WAVE_OK;
		//list_enq(wave_queue, &n);
		cell_queue_enq(&wave_queue, n);
	}
	/*for(int i=0; i<4; i++)
	{
		int index = next_to_gate[i];
		cell_t *n = &map[index];
		if(!is_good(n))
			cell_queue_enq(&wave_queue, n);
	}*/

	//send the wave out until we've marked all cells that
	//need to update
	//while(list_len(wave_queue))
	while(!cell_queue_isempty(&wave_queue))
	{
		//cell_t *n = *(cell_t **)list_deq(wave_queue);
		cell_t *n = cell_queue_deq(&wave_queue);
		assert(n);
		spread_wave(n);
	}
	/*for(int i=0; i<4; i++)
	{
		int index = next_to_gate[i];
		cell_t *n = &map[index];
		//if n is_good(), mark it, don't spread?
		spread_wave(n);
	}*/

	//backprop dists from wave ends
	//while(list_len(wave_ends_queue))
	while(!cell_queue_isempty(&wave_ends_queue))
	{
		//cell_t *n = *(cell_t **)list_deq(wave_ends_queue);
		cell_t *n = cell_queue_deq(&wave_ends_queue);
		assert(n);
		propagate_dists(n);
	}

	/*list_destroy(wave_queue);
	list_destroy(wave_ends_queue);
	wave_queue = NULL;
	wave_ends_queue = NULL;*/
}

void spread_wave(cell_t *n)
{
	//if n marked as bad, we already spread the wave from it
	//if(n->status == CELL_BAD)
	if(n->status != CELL_UNCHECKED && n->status != CELL_WAVE_OK)
		return;
	//n->status = CELL_WAVE_OK;
	if(is_good(n))
	{
		n->status = CELL_WAVE_END;
		//list_enq(wave_ends_queue, &n);
		cell_queue_enq(&wave_ends_queue, n);
		return;
	}

	n->status = CELL_BAD;
	cell_t *neighbors[4];
	int nei_len = get_neighbors(map, n, neighbors);
	for(int i=0; i<nei_len; i++)
		cell_queue_enq(&wave_queue, neighbors[i]);
		//list_enq(wave_queue, &neighbors[i]);
		//spread_wave(neighbors[i]);
}

void propagate_dists(cell_t *n)
{
	assert(n);
	//assert(n->dist < 20);

	//for(each neighbor nei)
	cell_t *nei[4];
	int nei_len = get_neighbors(map, n, nei);
	for(int i=0; i<nei_len; i++)
	{
		if(nei[i]->status == CELL_BAD)
		{
			if(nei[i]->dist < n->dist+1)
			{
				nei[i]->dist = n->dist+1;
				nei[i]->status = CELL_WAVE_OK;
				//list_enq(wave_ends_queue, &nei[i]);
				cell_queue_enq(&wave_ends_queue, nei[i]);
			}
		}
		else if(nei[i]->status == CELL_WAVE_OK)
		{
			if(nei[i]->dist > n->dist+1)
			{
				nei[i]->dist = n->dist+1;
			}
		}
	}

	//n->status = CELL_UNCHECKED;
}

bool is_good(cell_t *n)
{
	if(n->dist == 0)
		return true;
	if(n->status == CELL_BAD)
		return false;

	cell_t *nei[4];
	int nei_len = get_neighbors(map, n, nei);
	for(int i=0; i<nei_len; i++)
	{
		if(nei[i]->status != CELL_BAD
			&& nei[i]->status != CELL_WAVE_OK)
		{
			if(n->dist-1 == nei[i]->dist)
				return true;
		}
	}

	//no neighbors had n-1
	return false;

	/*
	if n marked as bad
		return false
	if n has a neighbor that is marked as good and has a n-1 neighbor
		return true
	return false
	*/
}

int get_neighbors(cell_t *map, cell_t *n, cell_t **nei)
{
	int len = 0;

	//
	int index = n - map;
	int x = index%9;
	int y = index/9;
	__int128 b = 1;
	b <<= index;

	//left
	//if(n->pred != NEIGHBOR_LEFT)
	if(!(x == 0 || blocked_left(b, vert)))
	{
		nei[len] = n-1;
	//	nei[len]->pred = NEIGHBOR_RIGHT;
		len++;
	}

	//right
	//if(n->pred != NEIGHBOR_RIGHT)
	if(!(x == 8 || blocked_right(b, vert)))
	{
		nei[len] = n+1;
	//	nei[len]->pred = NEIGHBOR_LEFT;
		len++;
	}

	//up
	//if(n->pred != NEIGHBOR_UP)
	if(!(y == 8 || blocked_up(b, horiz)))
	{
		nei[len] = n+9;
	//	nei[len]->pred = NEIGHBOR_DOWN;
		len++;
	}

	//down
	//if(n->pred != NEIGHBOR_DOWN)
	if(!(y == 0 || blocked_down(b, horiz)))
	{
		nei[len] = n-9;
	//	nei[len]->pred = NEIGHBOR_UP;
		len++;
	}

	return len;
}
