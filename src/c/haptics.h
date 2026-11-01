#pragma once

#include <pebble.h>

void haptics_init(void);
void haptics_deinit(void);
void haptics_set_enabled(bool on);
void haptics_set_quiet_hours(bool on);

void haptics_cast(void);
void haptics_bite(void);
void haptics_hook(void);
void haptics_warn(void);
void haptics_catch(void);
void haptics_snap(void);
void haptics_miss(void);
