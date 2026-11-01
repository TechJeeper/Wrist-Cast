#include "game.h"
#include "cast.h"
#include "haptics.h"
#include "ui.h"

#include <stdlib.h>
#include <string.h>

#define PERSIST_KEY_STATS 1
#define PERSIST_KEY_SETTINGS 2

typedef struct __attribute__((packed)) {
  uint16_t total_catches;
  uint8_t best_rarity;
  uint16_t streak;
} PersistStats;

typedef struct __attribute__((packed)) {
  uint8_t vibe_enabled;
  uint8_t quiet_hours;
  uint8_t wait_length;
  uint8_t cast_sensitivity;
  uint8_t fight_difficulty;
} PersistSettings;

static Game s_game;
static AppTimer *s_tick = NULL;

static const char *COMMON_NAMES[] = {
  "Bluegill", "Perch", "Minnow", "Sunfish", "Chub"
};
static const char *RARE_NAMES[] = {
  "Bass", "Trout", "Pike", "Catfish"
};
static const char *LEGEND_NAMES[] = {
  "Lake Leviathan", "Golden Koi", "Night Ray"
};

static void set_banner(const char *msg, uint16_t ms) {
  strncpy(s_game.banner, msg, sizeof(s_game.banner) - 1);
  s_game.banner[sizeof(s_game.banner) - 1] = '\0';
  s_game.banner_ms = ms;
}

static void save_stats(void) {
  PersistStats st = {
    .total_catches = s_game.total_catches,
    .best_rarity = s_game.best_rarity,
    .streak = s_game.streak,
  };
  persist_write_data(PERSIST_KEY_STATS, &st, sizeof(st));
}

static void save_settings(void) {
  PersistSettings ps = {
    .vibe_enabled = s_game.vibe_enabled ? 1 : 0,
    .quiet_hours = s_game.quiet_hours ? 1 : 0,
    .wait_length = s_game.wait_length,
    .cast_sensitivity = s_game.cast_sensitivity,
    .fight_difficulty = s_game.fight_difficulty,
  };
  persist_write_data(PERSIST_KEY_SETTINGS, &ps, sizeof(ps));
}

static void load_persist(void) {
  PersistStats st;
  if (persist_read_data(PERSIST_KEY_STATS, &st, sizeof(st)) == (int)sizeof(st)) {
    s_game.total_catches = st.total_catches;
    s_game.best_rarity = st.best_rarity;
    s_game.streak = st.streak;
  }
  PersistSettings ps;
  if (persist_read_data(PERSIST_KEY_SETTINGS, &ps, sizeof(ps)) == (int)sizeof(ps)) {
    s_game.vibe_enabled = ps.vibe_enabled != 0;
    s_game.quiet_hours = ps.quiet_hours != 0;
    s_game.wait_length = ps.wait_length > 2 ? 1 : ps.wait_length;
    s_game.cast_sensitivity = ps.cast_sensitivity > 2 ? 1 : ps.cast_sensitivity;
    s_game.fight_difficulty = ps.fight_difficulty > 2 ? 1 : ps.fight_difficulty;
  }
}

static uint16_t wait_range_ms(void) {
  // Returns max of range; min is half
  if (s_game.wait_length == 0) return 4000;
  if (s_game.wait_length == 2) return 18000;
  return 10000;
}

static void enter_ready(void) {
  s_game.state = GameState_Ready;
  s_game.state_ms = 0;
  s_game.tension = 50;
  s_game.reel_progress = 0;
  s_game.reel_fp = 0;
  s_game.up_held = false;
  s_game.down_held = false;
  s_game.out_of_green_ms = 0;
  cast_set_armed(true);
  set_banner("Flick to cast", 0);
}

static void pick_catch(void) {
  int roll = rand() % 100;
  FishRarity r;
  const char *name;
  if (roll < 8) {
    r = FishRarity_Legend;
    name = LEGEND_NAMES[rand() % ARRAY_LENGTH(LEGEND_NAMES)];
  } else if (roll < 35) {
    r = FishRarity_Rare;
    name = RARE_NAMES[rand() % ARRAY_LENGTH(RARE_NAMES)];
  } else {
    r = FishRarity_Common;
    name = COMMON_NAMES[rand() % ARRAY_LENGTH(COMMON_NAMES)];
  }
  s_game.catch_rarity = r;
  strncpy(s_game.catch_name, name, sizeof(s_game.catch_name) - 1);
  s_game.catch_name[sizeof(s_game.catch_name) - 1] = '\0';
}

static void enter_caught(void) {
  // catch_rarity / catch_name already set when the fight started
  s_game.state = GameState_Caught;
  s_game.state_ms = 0;
  cast_set_armed(false);
  s_game.total_catches++;
  s_game.streak++;
  if ((uint8_t)s_game.catch_rarity > s_game.best_rarity) {
    s_game.best_rarity = (uint8_t)s_game.catch_rarity;
  }
  save_stats();
  haptics_catch();
  set_banner("Caught!", 0);
}

static void enter_snapped(void) {
  s_game.state = GameState_Snapped;
  s_game.state_ms = 0;
  cast_set_armed(false);
  s_game.streak = 0;
  save_stats();
  haptics_snap();
  set_banner("Line snapped!", 0);
}

static void enter_fighting(void) {
  pick_catch(); // size/rarity locked in for this fight
  s_game.state = GameState_Fighting;
  s_game.state_ms = 0;
  s_game.tension = 50;
  s_game.reel_progress = 0;
  s_game.reel_fp = 0;
  s_game.up_held = false;
  s_game.down_held = false;
  s_game.out_of_green_ms = 0;
  cast_set_armed(false);
  haptics_hook();
  if (s_game.catch_rarity == FishRarity_Legend) {
    set_banner("Heavy pull!", 0);
  } else if (s_game.catch_rarity == FishRarity_Rare) {
    set_banner("Strong fish!", 0);
  } else {
    set_banner("Stay in green!", 0);
  }
}

static void enter_bite(void) {
  s_game.state = GameState_Bite;
  s_game.state_ms = 0;
  s_game.bite_window_ms = 1500;
  cast_set_armed(false);
  haptics_bite();
  set_banner("BITE! SELECT", 0);
}

static void enter_waiting(void) {
  s_game.state = GameState_Waiting;
  s_game.state_ms = 0;
  uint16_t max_ms = wait_range_ms();
  uint16_t min_ms = max_ms / 2;
  s_game.wait_ms = min_ms + (uint16_t)(rand() % (max_ms - min_ms + 1));
  cast_set_armed(false);
  haptics_cast();
  set_banner("Waiting…", 0);
}

uint8_t game_green_lo(void) {
  uint8_t lo = 30;
  if (s_game.fight_difficulty == 0) lo = 24;
  else if (s_game.fight_difficulty == 2) lo = 36;
  // Bigger fish → narrower green
  if (s_game.catch_rarity == FishRarity_Rare) lo = (uint8_t)(lo + 4);
  else if (s_game.catch_rarity == FishRarity_Legend) lo = (uint8_t)(lo + 8);
  return lo;
}

uint8_t game_green_hi(void) {
  uint8_t hi = 70;
  if (s_game.fight_difficulty == 0) hi = 76;
  else if (s_game.fight_difficulty == 2) hi = 64;
  if (s_game.catch_rarity == FishRarity_Rare) hi = (uint8_t)(hi - 4);
  else if (s_game.catch_rarity == FishRarity_Legend) hi = (uint8_t)(hi - 8);
  return hi;
}

bool game_in_green(void) {
  return s_game.tension >= (int16_t)game_green_lo()
      && s_game.tension <= (int16_t)game_green_hi();
}

static int16_t rarity_pull_bonus(void) {
  if (s_game.catch_rarity == FishRarity_Legend) return 2;
  if (s_game.catch_rarity == FishRarity_Rare) return 1;
  return 0;
}

static int16_t idle_drift_per_tick(void) {
  int16_t d = 1;
  if (s_game.fight_difficulty == 2) d = 2;
  return d + rarity_pull_bonus();
}

static int16_t reel_tension_per_tick(void) {
  int16_t d = 2;
  if (s_game.fight_difficulty == 0) d = 1;
  else if (s_game.fight_difficulty == 2) d = 3;
  return d + rarity_pull_bonus();
}

static int16_t ease_tension_per_tick(void) {
  int16_t d = 3;
  if (s_game.fight_difficulty == 2) d = 2;
  // Bigger fish fight the ease a bit
  if (s_game.catch_rarity == FishRarity_Legend) d = 2;
  return d;
}

static uint16_t snap_limit_ms(void) {
  uint16_t ms = 1800;
  if (s_game.fight_difficulty == 0) ms = 2600;
  else if (s_game.fight_difficulty == 2) ms = 1200;
  if (s_game.catch_rarity == FishRarity_Rare) ms = (uint16_t)(ms - 200);
  else if (s_game.catch_rarity == FishRarity_Legend) ms = (uint16_t)(ms - 400);
  if (ms < 800) ms = 800;
  return ms;
}

// Fixed-point reel fill: 10000 = full bar. Tuned slower overall;
// rarity stretches the fight (common ~8s, rare ~13s, legend ~20s on Normal).
static uint16_t reel_fp_gain_per_tick(void) {
  uint16_t base = 55; // normal common ≈ 9s of green time
  if (s_game.fight_difficulty == 0) base = 70;
  else if (s_game.fight_difficulty == 2) base = 40;

  if (s_game.catch_rarity == FishRarity_Rare) {
    base = (uint16_t)((base * 5) / 8);   // ~1.6× longer
  } else if (s_game.catch_rarity == FishRarity_Legend) {
    base = (uint16_t)((base * 2) / 5);   // ~2.5× longer
  }
  if (base < 12) base = 12;
  return base;
}

static void clamp_tension(void) {
  if (s_game.tension < 0) s_game.tension = 0;
  if (s_game.tension > 100) s_game.tension = 100;
}

static void fight_tick(void) {
  // Hold UP = tension rises (and reels a bit faster while green)
  // Hold DOWN = ease tension down
  // Neither = mild fish pull
  if (s_game.down_held && !s_game.up_held) {
    s_game.tension -= ease_tension_per_tick();
  } else if (s_game.up_held) {
    s_game.tension += reel_tension_per_tick();
  } else {
    s_game.tension += idle_drift_per_tick();
  }
  clamp_tension();

  // Progress whenever tension is in the green
  if (game_in_green()) {
    s_game.out_of_green_ms = 0;
    uint16_t gain = reel_fp_gain_per_tick();
    if (s_game.up_held) {
      gain = (uint16_t)((gain * 3) / 2); // +50% while actively pulling
    }
    if (s_game.reel_fp + gain >= 10000) {
      s_game.reel_fp = 10000;
      s_game.reel_progress = 100;
      s_game.up_held = false;
      s_game.down_held = false;
      enter_caught();
      return;
    }
    s_game.reel_fp = (uint16_t)(s_game.reel_fp + gain);
    s_game.reel_progress = (uint8_t)(s_game.reel_fp / 100);
  } else {
    s_game.out_of_green_ms += GAME_TICK_MS;
    if ((s_game.out_of_green_ms % 400) < GAME_TICK_MS) {
      haptics_warn();
    }
    if (s_game.out_of_green_ms >= snap_limit_ms()) {
      s_game.up_held = false;
      s_game.down_held = false;
      enter_snapped();
    }
  }
}

static void tick_cb(void *ctx) {
  (void)ctx;
  s_tick = app_timer_register(GAME_TICK_MS, tick_cb, NULL);
  game_tick();
}

void game_init(void) {
  memset(&s_game, 0, sizeof(s_game));
  s_game.state = GameState_Title;
  s_game.vibe_enabled = true;
  s_game.quiet_hours = false;
  s_game.wait_length = 1;
  s_game.cast_sensitivity = 1;
  s_game.fight_difficulty = 1;
  s_game.tension = 50;
  load_persist();

  haptics_init();
  haptics_set_enabled(s_game.vibe_enabled);
  haptics_set_quiet_hours(s_game.quiet_hours);
  cast_init();
  cast_set_sensitivity(s_game.cast_sensitivity);
  cast_set_armed(false);

  srand((unsigned)time(NULL));
  set_banner("SELECT to start", 0);
  s_tick = app_timer_register(GAME_TICK_MS, tick_cb, NULL);
}

void game_deinit(void) {
  if (s_tick) {
    app_timer_cancel(s_tick);
    s_tick = NULL;
  }
  cast_deinit();
  haptics_deinit();
  save_stats();
  save_settings();
}

void game_tick(void) {
  s_game.state_ms += GAME_TICK_MS;
  if (s_game.banner_ms > 0) {
    if (s_game.banner_ms <= GAME_TICK_MS) s_game.banner_ms = 0;
    else s_game.banner_ms -= GAME_TICK_MS;
  }

  switch (s_game.state) {
    case GameState_Waiting:
      if (s_game.state_ms >= s_game.wait_ms) {
        enter_bite();
      }
      break;

    case GameState_Bite:
      if (s_game.state_ms >= s_game.bite_window_ms) {
        haptics_miss();
        set_banner("Missed it", 1200);
        enter_ready();
      }
      break;

    case GameState_Fighting:
      fight_tick();
      break;

    default:
      break;
  }

  ui_mark_dirty();
}

const Game *game_get(void) {
  return &s_game;
}

void game_set_vibe_enabled(bool on) {
  s_game.vibe_enabled = on;
  haptics_set_enabled(on);
  save_settings();
}

void game_set_quiet_hours(bool on) {
  s_game.quiet_hours = on;
  haptics_set_quiet_hours(on);
  save_settings();
}

void game_set_wait_length(uint8_t v) {
  if (v > 2) v = 1;
  s_game.wait_length = v;
  save_settings();
}

void game_set_cast_sensitivity(uint8_t v) {
  if (v > 2) v = 1;
  s_game.cast_sensitivity = v;
  cast_set_sensitivity(v);
  save_settings();
}

void game_set_fight_difficulty(uint8_t v) {
  if (v > 2) v = 1;
  s_game.fight_difficulty = v;
  save_settings();
}

void game_start(void) {
  if (s_game.state == GameState_Title) {
    enter_ready();
  }
}

void game_cast(void) {
  if (s_game.state == GameState_Ready) {
    enter_waiting();
  }
}

void game_select(void) {
  switch (s_game.state) {
    case GameState_Title:
      game_start();
      break;
    case GameState_Ready:
      // Emulator / accessibility fallback
      game_cast();
      break;
    case GameState_Bite:
      enter_fighting();
      break;
    case GameState_Caught:
    case GameState_Snapped:
      enter_ready();
      break;
    default:
      break;
  }
}

void game_set_up_held(bool held) {
  s_game.up_held = held;
}

void game_set_down_held(bool held) {
  s_game.down_held = held;
}

void game_back(void) {
  switch (s_game.state) {
    case GameState_Title:
      // Let window_stack_pop_all via click handler — nothing here
      break;
    case GameState_Fighting:
    case GameState_Waiting:
    case GameState_Bite:
    case GameState_Caught:
    case GameState_Snapped:
      enter_ready();
      break;
    case GameState_Ready:
      s_game.state = GameState_Title;
      cast_set_armed(false);
      set_banner("SELECT to start", 0);
      break;
  }
}
