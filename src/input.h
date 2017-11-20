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

bool input_button_down(input_button_t b);

bool input_button_clicked(input_button_t b);

bool input_dragging(input_button_t b);
void input_drag_delta(input_button_t b, float* x, float* y);

void input_position(float* x, float* y);
void input_position_delta(float* dx, float* dy);
