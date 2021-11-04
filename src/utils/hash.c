#include <stdint.h>
#include "hash.h"

int hashbytes(unsigned char *str, int len)
{
	int x = 0; // Temp?

	x ^= *str << 7;

	for(int i = 0; i < len; i += 1)
		x = (1000003UL * x) ^ *str++;

	x ^= len;

	if(x == -1)
		x = -2;
	
	return x;
}