
typedef struct
{
	uint8_t dist;
	cell_status_t status;
} cell_t;

cell_t map[81];

void map_init(cell_t *map)
{
	for(int i=0; i<81; i++)
	{
		map[i].dist = i/9;
		map[i].status = UNCHECKED;
	}
}

list_t all_wave_ends = NULL;

update_dists(cell_t *map, int *next_to_gate)
{
	//init list
	if(!all_wave_ends)
		all_wave_ends = list(cell_t);

	//mark every cell as good
	for(int i=0; i<81; i++)
		map[i].status = UNCHECKED;

	//for each n next to gate, spread_wave
	for(int i=0; i<4; i++)
	{
		int index = next_to_gate[i];
		cell_t *n = map[index];
		//if n is_good(), mark it, don't spread?
		spread_wave(n);
	}

	//backprop dists from wave ends
	while(list_len(all_wave_ends))
	{
		cell_t *n = list_dequeue(all_wave_ends);
		propagate_dists(n);
	}

	list_destroy(all_wave_ends);
	all_wave_ends = NULL;
}

void spread_wave(cell_t *n)
{
	//if n marked as bad, we already spread the wave from it
	//if(n->status == CELL_BAD)
	if(n->status != UNCHECKED)
		return;

	if(is_good(n))
	{
		n->status = WAVE_END;
		list_enqueue(n);
	}

	n->status = CELL_BAD;
	for(each neighbor nei)
		spread_wave(nei)

	/*
	//if n has a good neighbor, it's a wave end
	for(each neighbor)
	{
		if(nei.status == WAVE_END)
			continue;	//already checked
		if(nei->status == UNCHECKED)
		{
			nei.status = WAVE_END;
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

void propagate_dists(n)
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

	/*for each neighbor
		if nei marked as bad
			if(nei.score > n.score-1)
				nei.score = n-1
				enqueue(nei)
	*/
}

bool is_good(cell_t *n)
{
	if(n->dist == 0)
		return true;
	if(n->status == CELL_BAD)
		return false;
	for(each neighbor nei)
	{
		if(nei->status == ?)
		{
			for(each neighbor neinei)
			{
				if(nei->score-1 == neinei->score)
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
