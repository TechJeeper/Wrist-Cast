#include "cast.h"
#include "game.h"

static bool s_armed = false;
static uint8_t s_sensitivity = 1;
static int16_t s_prev_x = 0;
static int16_t s_prev_y = 0;
static int16_t s_prev_z = 0;
static bool s_have_prev = false;
static uint16_t s_cooldown_ms = 0;

static int32_t threshold_for(uint8_t level) {
  // Magnitude of delta accel (rough milli-g units from Pebble samples)
  if (level == 0) return 1400; // low sensitivity → harder to cast
  if (level == 2) return 700;  // high → easier
  return 1000;
}

static void accel_handler(AccelData *data, uint32_t num_samples) {
  if (!s_armed || !data || num_samples == 0) return;

  // Cooldown ticks down roughly at sample rate (~50Hz → 20ms)
  if (s_cooldown_ms > 0) {
    uint16_t step = (uint16_t)(num_samples * 20);
    if (step > s_cooldown_ms) s_cooldown_ms = 0;
    else s_cooldown_ms -= step;
  }

  for (uint32_t i = 0; i < num_samples; i++) {
    if (data[i].did_vibrate) continue;
    int16_t x = data[i].x;
    int16_t y = data[i].y;
    int16_t z = data[i].z;
    if (!s_have_prev) {
      s_prev_x = x;
      s_prev_y = y;
      s_prev_z = z;
      s_have_prev = true;
      continue;
    }
    int32_t dx = (int32_t)x - s_prev_x;
    int32_t dy = (int32_t)y - s_prev_y;
    int32_t dz = (int32_t)z - s_prev_z;
    s_prev_x = x;
    s_prev_y = y;
    s_prev_z = z;

    int32_t mag2 = dx * dx + dy * dy + dz * dz;
    int32_t thr = threshold_for(s_sensitivity);
    if (s_cooldown_ms == 0 && mag2 >= thr * thr) {
      s_cooldown_ms = 800;
      game_cast();
      return;
    }
  }
}

void cast_init(void) {
  s_armed = false;
  s_sensitivity = 1;
  s_have_prev = false;
  s_cooldown_ms = 0;
  accel_data_service_subscribe(5, accel_handler);
  accel_service_set_sampling_rate(ACCEL_SAMPLING_50HZ);
}

void cast_deinit(void) {
  accel_data_service_unsubscribe();
  s_armed = false;
}

void cast_set_sensitivity(uint8_t level) {
  if (level > 2) level = 1;
  s_sensitivity = level;
}

void cast_set_armed(bool armed) {
  s_armed = armed;
  s_have_prev = false;
  s_cooldown_ms = 0;
}
