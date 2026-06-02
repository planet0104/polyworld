#pragma once

// POSIX drand48/srand48 are used for global RNG; not provided by MinGW UCRT.
#if defined(_WIN32) && !defined(__CYGWIN__)

void srand48(long seedval);
double drand48(void);

#endif
