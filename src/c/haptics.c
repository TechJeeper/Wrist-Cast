#include "haptics.h"

static bool s_enabled = true;
static bool s_quiet = false;

static bool quiet_now(void) {
  if (!s_quiet) return false;
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  int hour = t->tm_hour;
  // Quiet 10pm–7am
  return (hour >= 22 || hour < 7);
}

static bool can_vibe(void) {
  return s_enabled && !quiet_now();
}

static void pulse(const uint32_t *seg, uint32_t n) {
  if (!can_vibe()) return;
  VibePattern p = { .durations = (uint32_t *)seg, .num_segments = n };
  vibes_enqueue_custom_pattern(p);
}

void haptics_init(void) {
  s_enabled = true;
  s_quiet = false;
}

void haptics_deinit(void) {
  vibes_cancel();
}

void haptics_set_enabled(bool on) {
  s_enabled = on;
  if (!on) vibes_cancel();
}

void haptics_set_quiet_hours(bool on) {
  s_quiet = on;
}

void haptics_cast(void) {
  static const uint32_t seg[] = { 40, 30, 80 };
  pulse(seg, 3);
}

void haptics_bite(void) {
  static const uint32_t seg[] = { 60, 40, 60, 40, 120 };
  pulse(seg, 5);
}

void haptics_hook(void) {
  static const uint32_t seg[] = { 30, 20, 30, 20, 80 };
  pulse(seg, 5);
}

void haptics_warn(void) {
  static const uint32_t seg[] = { 25 };
  pulse(seg, 1);
}

void haptics_catch(void) {
  static const uint32_t seg[] = { 50, 40, 50, 40, 50, 40, 140 };
  pulse(seg, 7);
}

void haptics_snap(void) {
  static const uint32_t seg[] = { 200, 60, 80 };
  pulse(seg, 3);
}

void haptics_miss(void) {
  static const uint32_t seg[] = { 35, 50, 35 };
  pulse(seg, 3);
}
