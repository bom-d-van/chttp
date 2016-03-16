#ifndef _debug_h
#define _debug_h

#ifdef NDEBUG
	#define dprintf(M, ...)
#else
	#define dprintf(M, ...) printf(M, ##__VA_ARGS__)
#endif

#endif
