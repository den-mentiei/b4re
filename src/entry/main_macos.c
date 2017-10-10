#ifdef BR_PLATFORM_MACOS

#include "entry.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "log.h"
#include "timer.h"

int main(int argc, const char* argv[]) {
	log_info("hello apple world!\n");
	return 0;
}

#endif
