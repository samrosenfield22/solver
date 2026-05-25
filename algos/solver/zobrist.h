

#ifndef ZOBRIST_H_
#define ZOBRIST_H_

#include <stdint.h>

void zobrist_init(int zseed);
void zobrist_place(uint32_t *h, int n);

#endif	//ZOBRIST_H_
