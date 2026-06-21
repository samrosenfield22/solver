

#ifndef ZOBRIST_H_
#define ZOBRIST_H_

#include <stdint.h>

void zobrist_init(int len, int zseed);
void zobrist_free(void);
void zobrist_place(uint64_t *h, int n);
void zobrist_move(uint64_t *h, int to, int from);

#endif	//ZOBRIST_H_
