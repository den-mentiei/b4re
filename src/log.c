#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define PRINT_TO(s, f)    \
	va_list args;         \
	va_start(args, f);    \
	vfprintf(s, f, args); \
	va_end(args);

void log_info(const char* format, ...) {
	PRINT_TO(stdout, format);
}

void log_error(const char* format, ...) {
	PRINT_TO(stderr, format);
}

void log_fatal(const char* format, ...) {
	PRINT_TO(stderr, format);
	fflush(NULL);
	exit(1);
}
