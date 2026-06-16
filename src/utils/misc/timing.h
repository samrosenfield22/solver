/**/

#ifndef TIMING_H_
#define TIMING_H_

//#include <stdio.h>
#include <inttypes.h>


//
void tic(void);
uint64_t toc(void);
uint32_t toc_ms(void);
uint32_t toc_s(void);
void delay(int ms);


#endif
