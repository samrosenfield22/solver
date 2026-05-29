

#include "../../../utils.h"
#include "quoridor_pathfind.h"

//
void spread_wave(cell_t *n);
void propagate_dists(cell_t *n);
bool is_good(cell_t *n);
int get_neighbors(cell_t *map, cell_t *n, cell_t **nei);

cell_t *map = NULL;
__int128 horiz, vert;

void map_init(cell_t *m)
{
	for(int i=0; i<81; i++)
	{
		m[i].dist = i/9;
		m[i].status = CELL_UNCHECKED;
	}
}

list_t *wave_ends_queue = NULL;

void update_dists(cell_t *m, int *next_to_gate,
	__int128 h, __int128 v)
{
	map = m;
	horiz = h;
	vert = v;

	//init list
	if(!wave_ends_queue)
		wave_ends_queue = list(cell_t);

	//mark every cell as good
	for(int i=0; i<81; i++)
		map[i].status = CELL_UNCHECKED;

	//for each n next to gate, spread_wave
	for(int i=0; i<4; i++)
	{
		int index = next_to_gate[i];
		cell_t *n = &map[index];
		//if n is_good(), mark it, don't spread?
		spread_wave(n);
	}

	//backprop dists from wave ends
	/*while(list_len(wave_ends_queue))
	{
		cell_t *n = list_dequeue(wave_ends_queue);
		propagate_dists(n);
	}*/

	list_destroy(wave_ends_queue);
	wave_ends_queue = NULL;
}

void spread_wave(cell_t *n)
{
	//if n marked as bad, we already spread the wave from it
	//if(n->status == CELL_BAD)
	if(n->status != CELL_UNCHECKED)
		return;

	if(is_good(n))
	{
		n->status = CELL_WAVE_END;
		list_enq(wave_ends_queue, n);
	}

	n->status = CELL_BAD;
	cell_t *neighbors[4];
	int nei_len = get_neighbors(map, n, neighbors);
	for(int i=0; i<nei_len; i++)
		spread_wave(neighbors[i]);

	/*
	//if n has a good neighbor, it's a wave end
	for(each neighbor)
	{
		if(nei.status == CELL_WAVE_END)
			continue;	//already checked
		if(nei->status == CELL_UNCHECKED)
		{
			nei.status = CELL_WAVE_END;
			list_enqueue(nei);
		}
		else
			spread_wave(nei);
	}

	if n marked as bad
		return (already done)
	for each neighbor
		if(is_good(nei))
			enqueue(nei)
			return
	mark n as bad
	for each neighbor
		spread_wave(nei)
	*/
}

/*void propagate_dists(cell_t *n)
{
	for(each neighbor nei)
	{
		if(nei->status == CELL_BAD)
		{
			if(nei->score > n->score-1)
			{
				nei.score = n->score-1;
				list_enqueue(nei);
			}
		}
	}
}*/

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
		if(nei[i]->status != CELL_BAD)
		{
			cell_t *neinei[4];
			int neinei_len = get_neighbors(map, nei[i], neinei);
			for(int j=0; j<neinei_len; j++)
			{
				if(nei[i]->dist-1 == neinei[j]->dist)
					return true;
			}
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
	if(!(x == 0 || blocked_left(b, vert)))
	{
		nei[len] = n-1;
		len++;
	}

	//right
	if(!(x == 8 || blocked_right(b, vert)))
	{
		nei[len] = n+1;
		len++;
	}

	//up
	if(!(y == 8 || blocked_up(b, horiz)))
	{
		nei[len] = n+9;
		len++;
	}

	//down
	if(!(y == 0 || blocked_down(b, horiz)))
	{
		nei[len] = n-9;
		len++;
	}

	return len;
}
