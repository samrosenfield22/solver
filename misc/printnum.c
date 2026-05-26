

#include "printnum.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

//


/*void print128(__int128 x)
{
	//printbig(x, 16, ' ', 4);
	printbig(x, "%x");
}*/

/*valid format options:
d:		print decimal
x/X: 	print hex
b:		print binary
0:		prepend w up to 9 leading zeros
+:		prepend w + for positive
*/
int printbig(__int128 x, const char *fmt)
{
	char *buf = sprintbig(x, fmt);
	int len = strlen(buf);
	printf("%s", buf);
	return len;
}

char *sprintbig(__int128 x, const char *fmt)
{
	bool fmt_valid = true;

	//params
	int base = 0;
	char sep = 0;
	int digits_per_sep = 0;
	int leading_zeros = 0;
	bool show_plus = false;
	bool capital = false;

	if(!fmt)
		goto fmt_confirm;
	if(*fmt != '%')
	{
		fmt_valid = false;
		goto fmt_confirm;
	}

	//parse format
	for(const char *c=fmt+1; *c; c++)
	{
		switch(*c)
		{
			case '0':
				//this doesnt handle more than 9 leadings
				c++;
				if(!('0' <= *c && *c <= '9'))
					goto fmt_confirm;
				leading_zeros = *c - '0';
				break;

			case '+':
				show_plus = true;
				break;

			case 'x':
			case 'X':
				base = 16;
				sep = ' ';
				digits_per_sep = 4;
				if(*c=='X')
					capital = true;
				break;

			case 'd':
			case 'u':
				base = 10;
				sep = ',';
				digits_per_sep = 3;
				break;

			case 'b':
				base = 2;
				sep = ' ';
				digits_per_sep = 8;
				break;

			default:
				fmt_valid = false;
				break;

		}
	}

	if(!base)
		fmt_valid = false;

	fmt_confirm:
	if(!fmt_valid)
	{
		printf("error: invalid print format \'%s\'\n", fmt);
		exit(0);
	}

	return printbig_custom(x, base, sep, digits_per_sep,
		leading_zeros, show_plus, capital);
}

char *printbig_custom(__int128 x, int base, char sep,
	int digits_per_sep, int leading_zeros, bool show_plus,
	bool capital)
{
	static char buf[138] = "\0";
	char *bc = buf;

	if(x < 0)
	{
		//putchar('-');
		*bc++ = '-';
		x = 0-x;
	}
	else if(show_plus)
	{
		//putchar('+');
		*bc++ = '+';
	}

	if(x == 0)
	{
		//putchar('0');
		*bc++ = '0';
		//return;
		goto print_custom_end;
	}

	int max_dig;
	__int128 weight = 1;
	for(int i=0;; i++)
	{
		weight *= base;
		if(weight * (__int128)base < weight)
		{
			max_dig = i+1;
			break;
		}
	}

	bool started = false;
	for(int i=max_dig; i>=0; i--)
	{
		//__int128 d = (x>>(i*4));
		__int128 d = x / weight;
		if(d)
		{
			if((i % digits_per_sep)==(digits_per_sep-1)
				&& started)
				//putchar(sep);
				*bc++ = sep;
			{
				started = true;
				for(int z=0; z<leading_zeros-i; z++)
					*bc++ = '0';
					//putchar('0');
			}

			int digit = d % base;
			if(digit < 10)
				*bc++ = '0'+digit;
				//putchar('0'+digit);
			else
			{
				char c = digit - 0xA;
				c += capital? 'A' : 'a';
				*bc++ = c;
				//putchar(c);
			}
		}

		weight /= base;
		//if(!weight)
		//	break;
	}

	assert(!weight);

	print_custom_end:
	*bc = '\0';
	return buf;
}
