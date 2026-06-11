

#ifndef CLOCK_H_
#define CLOCK_H_

#include <stdbool.h>

void clocks_init(int p1, int p2);
int clock_get_time(void);
void clock_resume(bool who);
bool clock_update(void);
bool clock_flag(void);

#endif	//CLOCK_H_
