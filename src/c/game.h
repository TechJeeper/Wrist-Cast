#pragma once

#include <pebble.h>

#define GAME_TICK_MS 50

typedef enum {
  GameState_Title = 0,
  GameState_Ready,
  GameState_Waiting,
  GameState_Bite,
  GameState_Fighting,
  GameState_Caught,
  GameState_Snapped,
} GameState;

typedef enum {
  FishRarity_Common = 0,
  FishRarity_Rare,
  FishRarity_Legend,
} FishRarity;

typedef struct {
  GameState state;
  uint16_t state_ms;
  uint16_t wait_ms;          // scheduled wait before bite
  uint16_t bite_window_ms;
  int16_t tension;           // 0–100
  uint8_t reel_progress;     // 0–100
  bool up_held;
  bool down_held;
  uint16_t out_of_green_ms;
  FishRarity catch_rarity;
  char catch_name[20];
  char banner[28];
  uint16_t banner_ms;
  uint16_t total_catches;
  uint8_t best_rarity;
  uint16_t streak;
  // Settings mirrors
  bool vibe_enabled;
  bool quiet_hours;
  uint8_t wait_length;       // 0 short, 1 normal, 2 long
  uint8_t cast_sensitivity;  // 0 low, 1 normal, 2 high
  uint8_t fight_difficulty;  // 0 easy, 1 normal, 2 hard
} Game;

void game_init(void);
void game_deinit(void);
void game_tick(void);
const Game *game_get(void);

void game_set_vibe_enabled(bool on);
void game_set_quiet_hours(bool on);
void game_set_wait_length(uint8_t v);
void game_set_cast_sensitivity(uint8_t v);
void game_set_fight_difficulty(uint8_t v);

void game_start(void);
void game_cast(void);
void game_select(void);
void game_set_up_held(bool held);
void game_set_down_held(bool held);
void game_back(void);

bool game_in_green(void);
uint8_t game_green_lo(void);
uint8_t game_green_hi(void);
