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

#include <stdio.h>
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

char *now_hms(void)
{
	static char timestr[10];

	time_t raw_time;
    struct tm *local_time;

    // Get time as seconds since epoch
    time(&raw_time);

    // Convert to local time structure
    local_time = localtime(&raw_time);

    // Print formatted time (HH:MM:SS)
    snprintf(timestr, 9, "%02d:%02d:%02d",
            local_time->tm_hour,
            local_time->tm_min,
            local_time->tm_sec);

	return timestr;
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
