#pragma once

#include <pebble.h>

void cast_init(void);
void cast_deinit(void);
void cast_set_sensitivity(uint8_t level); // 0 low, 1 normal, 2 high
void cast_set_armed(bool armed);
