

#ifndef CLOCK_H_
#define CLOCK_H_

#include <stdbool.h>

void clocks_init(int p1, int p2);
void clock_resume(bool who);
bool clock_update(bool player);

#endif	//CLOCK_H_
