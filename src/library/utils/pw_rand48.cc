#if defined(_WIN32) && !defined(__CYGWIN__)

#include "pw_rand48.h"

#include <math.h>

static unsigned short pw_rand48_state[3];

static double erand48(unsigned short xsubi[3])
{
	for (int i = 0; i < 7; i++)
	{
		unsigned long long r = (unsigned long long)0x5deece66dULL * xsubi[0] + xsubi[2];
		xsubi[0] = (unsigned short)r;
		r >>= 16;
		r = (unsigned long long)0x5deece66dULL * r + xsubi[1];
		xsubi[1] = (unsigned short)r;
		r >>= 16;
		r = (unsigned long long)0x5deece66dULL * r + 0xb;
		xsubi[2] = (unsigned short)r;
	}
	return ldexp((double)xsubi[0], -48)
	     + ldexp((double)xsubi[1], -32)
	     + ldexp((double)xsubi[2], -16);
}

void srand48(long seedval)
{
	pw_rand48_state[0] = 0x330e;
	pw_rand48_state[1] = (unsigned short)(seedval & 0xffff);
	pw_rand48_state[2] = (unsigned short)((unsigned long)seedval >> 16);
}

double drand48(void)
{
	return erand48(pw_rand48_state);
}

#endif
