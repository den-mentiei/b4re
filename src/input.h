#pragma once

#include <stdbool.h>

void input_update();

typedef enum {
	INPUT_BUTTON_LEFT = 0,
	INPUT_BUTTON_RIGHT
} input_button_t;

// Returns *true* if the user pressed the button during this frame, of *false* otherwise.
bool input_button_pressed(input_button_t b);

// Returns *true* if the user released the button during this frame, of *false* otherwise.
bool input_button_released(input_button_t b);

void input_position(float* x, float* y);
