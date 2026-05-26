

#ifndef ZOBRIST_H_
#define ZOBRIST_H_

#include <stdint.h>

void zobrist_init(int zseed);
void zobrist_place(uint32_t *h, int n);
void zobrist_move(uint32_t *h, int to, int from);

#endif	//ZOBRIST_H_
