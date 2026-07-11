

#ifndef BITBOARD_H_
#define BITBOARD_H_

#include <stdint.h>
#include <stdbool.h>

#include "solver.h"

//
void bb64_init(int w, int h, uint64_t mask);
uint64_t bb64_get_open(uint64_t bb);
int bb64_get_open_ct(uint64_t bb);
bool bb64_is_full(uint64_t bb);
bool bb64_is_empty(uint64_t bb);
uint64_t bb64_place(uint64_t bb, uint64_t bit, uint64_t *hash, bool whosemove);
uint64_t bb64_hash(uint64_t bb, bool whosemove);
int bb64_make_place_movelist(sorter_t *sorter, uint64_t bb);

#endif	//BITBOARD_H_
