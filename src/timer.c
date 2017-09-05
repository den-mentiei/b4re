#include "timer.h"

#if BR_PLATFORM_LINUX
	#include <time.h> // clock_gettime
#else
	#error "Not implemented yet."
#endif

double timer_current() {
	#if BR_PLATFORM_LINUX
		struct timespec t;
		clock_gettime(CLOCK_REALTIME, &t);
		return t.tv_sec * 1000.0 + t.tv_nsec / 1000000.0;
	#else
		#error "Not implemented yet."
	#endif
}
