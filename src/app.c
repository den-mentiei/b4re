#include "entry/entry.h"

#include <stdio.h>

bool entry_init(int argc, const char* argv[]) {
	printf("hello!\n");
	return true;
}

bool entry_tick() {
	return true;
}

void entry_shutdown() {
	printf("goodbye\n");
}
