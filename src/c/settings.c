#include "settings.h"
#include "game.h"

#include <stdlib.h>

static int32_t tuple_to_int(Tuple *t) {
  if (!t) return -1;
  if (t->type == TUPLE_INT) return t->value->int32;
  if (t->type == TUPLE_UINT) return (int32_t)t->value->uint32;
  if (t->type == TUPLE_CSTRING) return (int32_t)atoi(t->value->cstring);
  return -1;
}

static void inbox_received(DictionaryIterator *it, void *context) {
  (void)context;

  Tuple *vibe = dict_find(it, MESSAGE_KEY_VibeEnabled);
  if (vibe) {
    int32_t v = tuple_to_int(vibe);
    if (v >= 0) game_set_vibe_enabled(v != 0);
  }

  Tuple *quiet = dict_find(it, MESSAGE_KEY_QuietHours);
  if (quiet) {
    int32_t v = tuple_to_int(quiet);
    if (v >= 0) game_set_quiet_hours(v != 0);
  }

  Tuple *wait = dict_find(it, MESSAGE_KEY_WaitLength);
  if (wait) {
    int32_t v = tuple_to_int(wait);
    if (v >= 0) game_set_wait_length((uint8_t)v);
  }

  Tuple *sens = dict_find(it, MESSAGE_KEY_CastSensitivity);
  if (sens) {
    int32_t v = tuple_to_int(sens);
    if (v >= 0) game_set_cast_sensitivity((uint8_t)v);
  }

  Tuple *diff = dict_find(it, MESSAGE_KEY_FightDifficulty);
  if (diff) {
    int32_t v = tuple_to_int(diff);
    if (v >= 0) game_set_fight_difficulty((uint8_t)v);
  }
}

void settings_init(void) {
  app_message_register_inbox_received(inbox_received);
  app_message_open(128, 64);
}

void settings_deinit(void) {
}
