#pragma once

void log_info(const char* format, ...);

void log_error(const char* format, ...);

// Exits immediately.
void log_fatal(const char* format, ...);
