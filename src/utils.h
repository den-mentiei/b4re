#pragma once

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define MIN(x, y)     (((x) < (y)) ? (x) : (y))
#define MAX(x, y)     (((x) > (y)) ? (x) : (y))
#define CLAMP(v,l,h)  MIN(MAX((v), (l)), (h))
