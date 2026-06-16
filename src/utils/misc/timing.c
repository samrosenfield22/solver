/**/
/*
	This timing library includes public functions:
	* tic() saves the current time (in microseconds) to an internal counter
	* toc() returns the time that has elapsed since	the last call of tic().
	* toc_ms() and toc_s() convert to ms and s
	* delay() pauses (given in milliseconds)

	ex.
	tic();
	functionThatTakesALongTime();
	uint64_t elapsedTime = toc();
	printf("elapsed time: %" PRIu64 " us (%lf seconds)\n", elapsedTime, ((float)(elapsedTime))/1000000 );
*/

#include "timing.h"

#include <time.h>
#include <windows.h>

static uint64_t TICTOC_STOPWATCH = 0ULL;

//
static uint64_t getSystemTime(void);

void tic()
{
	TICTOC_STOPWATCH = getSystemTime();
}

uint64_t toc()
{
	uint64_t elapsed = (getSystemTime() - TICTOC_STOPWATCH) / 10;
	return elapsed;
}


uint32_t toc_ms(void)
{
	return toc()/1000;
}

uint32_t toc_s(void)
{
	return toc()/1000000;
}

void delay(int ms)
{
	tic();
	while(toc_ms() < ms);
}

//returns the number of 1/10th-microsecond (100-nanosecond) counts since ???
static uint64_t getSystemTime()
{
	FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    uint64_t timeUs = ft.dwHighDateTime;
    timeUs = (timeUs<<32) | ft.dwLowDateTime;
    //timeUs /= 10;

    return timeUs;
}
