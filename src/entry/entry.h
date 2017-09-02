#pragma once

#include <stdbool.h>

bool entry_init(int argc, const char* argv[]);
bool entry_tick();
void entry_shutdown();
