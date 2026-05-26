

#ifndef PRINTNUM_H_
#define PRINTNUM_H_

#include <stdbool.h>

int printbig(__int128 x, const char *fmt);
char *sprintbig(__int128 x, const char *fmt);
char *printbig_custom(__int128 x, int base, char sep,
	int digits_per_sep, int leading_zeros, bool show_plus,
	bool capital);

#endif	//PRINTNUM_H_
