#include "ui.h"
#include "game.h"

static Window *s_window;
static Layer *s_canvas;
static AppTimer *s_redraw = NULL;

static GBitmap *s_bmp_title = NULL;
static GBitmap *s_bmp_water = NULL;
static GBitmap *s_bmp_rod = NULL;
static GBitmap *s_bmp_bobber = NULL;
static GBitmap *s_bmp_fish_common = NULL;
static GBitmap *s_bmp_fish_rare = NULL;
static GBitmap *s_bmp_fish_legend = NULL;

#if defined(PBL_COLOR)
#define C_BG      GColorBlack
#define C_WATER   GColorDukeBlue
#define C_SKY     GColorBlack
#define C_TEXT    GColorWhite
#define C_DIM     GColorLightGray
#define C_GREEN   GColorIslamicGreen
#define C_YELLOW  GColorChromeYellow
#define C_RED     GColorRed
#define C_ACCENT  GColorCyan
#else
#define C_BG      GColorBlack
#define C_WATER   GColorBlack
#define C_SKY     GColorBlack
#define C_TEXT    GColorWhite
#define C_DIM     GColorWhite
#define C_GREEN   GColorWhite
#define C_YELLOW  GColorWhite
#define C_RED     GColorWhite
#define C_ACCENT  GColorWhite
#endif

static void redraw_cb(void *ctx) {
  (void)ctx;
  s_redraw = NULL;
  if (s_canvas) layer_mark_dirty(s_canvas);
  s_redraw = app_timer_register(100, redraw_cb, NULL);
}

// NOTE: On device, graphics_draw_bitmap_in_rect tiles when dest is larger than
// the source — never upscale sprites. Stretching title/water is OK visually
// as a backdrop, but rods/fish must draw 1:1.
static void draw_bitmap_in(GContext *ctx, GBitmap *bmp, GRect dest) {
  if (!bmp) return;
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_draw_bitmap_in_rect(ctx, bmp, dest);
}

static void draw_bitmap_native(GContext *ctx, GBitmap *bmp, GPoint origin) {
  if (!bmp) return;
  GRect b = gbitmap_get_bounds(bmp);
  draw_bitmap_in(ctx, bmp, GRect(origin.x, origin.y, b.size.w, b.size.h));
}

static void draw_bitmap_centered_native(GContext *ctx, GBitmap *bmp, GPoint center) {
  if (!bmp) return;
  GRect b = gbitmap_get_bounds(bmp);
  draw_bitmap_native(ctx, bmp, GPoint(center.x - b.size.w / 2, center.y - b.size.h / 2));
}

static GBitmap *fish_bmp_for(FishRarity r) {
  if (r == FishRarity_Legend) return s_bmp_fish_legend;
  if (r == FishRarity_Rare) return s_bmp_fish_rare;
  return s_bmp_fish_common;
}

static const char *rarity_label(FishRarity r) {
  if (r == FishRarity_Legend) return "LEGEND";
  if (r == FishRarity_Rare) return "Rare";
  return "Common";
}

static void draw_scene(GContext *ctx, GRect bounds, const Game *g) {
  // Horizon sits higher so water fills most of the display
  int16_t water_y = bounds.size.h * 2 / 5;

  graphics_context_set_fill_color(ctx, C_SKY);
  graphics_fill_rect(ctx, GRect(0, 0, bounds.size.w, water_y), 0, GCornerNone);

  if (s_bmp_water) {
    GRect wb = gbitmap_get_bounds(s_bmp_water);
    int16_t wy = water_y - 2;
    // Tile water 1:1 across width (no upscale — avoids 2x2 sprite repeats)
    for (int16_t x = 0; x < bounds.size.w; x += wb.size.w) {
      draw_bitmap_native(ctx, s_bmp_water, GPoint(x, wy));
    }
    graphics_context_set_fill_color(ctx, C_WATER);
    graphics_fill_rect(ctx, GRect(0, wy + wb.size.h, bounds.size.w,
                                  bounds.size.h - (wy + wb.size.h)), 0, GCornerNone);
  } else {
    graphics_context_set_fill_color(ctx, C_WATER);
    graphics_fill_rect(ctx, GRect(0, water_y, bounds.size.w, bounds.size.h - water_y),
                       0, GCornerNone);
    graphics_context_set_stroke_color(ctx, C_ACCENT);
    graphics_draw_line(ctx, GPoint(0, water_y), GPoint(bounds.size.w, water_y));
  }

  if (s_bmp_rod) {
    GRect rb = gbitmap_get_bounds(s_bmp_rod);
    draw_bitmap_native(ctx, s_bmp_rod,
                       GPoint(bounds.size.w - rb.size.w - 4, water_y - rb.size.h + 8));
  }

  int16_t bobber_x = bounds.size.w / 2;
  int16_t bobber_y = water_y + bounds.size.h / 10;
  if (g->state == GameState_Waiting) {
    bobber_y += (int16_t)((g->state_ms / 200) % 2) * 3;
  } else if (g->state == GameState_Bite) {
    bobber_y += 10 + (int16_t)((g->state_ms / 80) % 3) * 2;
  }

  if (g->state == GameState_Waiting || g->state == GameState_Bite ||
      g->state == GameState_Ready) {
    graphics_context_set_stroke_color(ctx, C_DIM);
    graphics_context_set_stroke_width(ctx, 1);
    graphics_draw_line(ctx, GPoint(bounds.size.w - 28, water_y - 20),
                       GPoint(bobber_x, bobber_y));
    if (s_bmp_bobber) {
      draw_bitmap_centered_native(ctx, s_bmp_bobber, GPoint(bobber_x, bobber_y + 8));
    } else {
      graphics_context_set_fill_color(ctx, C_RED);
      graphics_fill_circle(ctx, GPoint(bobber_x, bobber_y + 8), 6);
    }
  }
}

static void draw_tension(GContext *ctx, GRect bounds, const Game *g) {
  int16_t bar_x = 8;
  int16_t bar_w = bounds.size.w - 16;
  int16_t bar_y = bounds.size.h / 2 - 16;
  int16_t bar_h = 28;

  graphics_context_set_fill_color(ctx, C_DIM);
  graphics_fill_rect(ctx, GRect(bar_x, bar_y, bar_w, bar_h), 2, GCornersAll);

  uint8_t lo = game_green_lo();
  uint8_t hi = game_green_hi();
  int16_t green_x = bar_x + (int16_t)(bar_w * lo / 100);
  int16_t green_w = (int16_t)(bar_w * (hi - lo) / 100);
  graphics_context_set_fill_color(ctx, C_GREEN);
  graphics_fill_rect(ctx, GRect(green_x, bar_y, green_w, bar_h), 0, GCornerNone);

  int16_t needle = bar_x + (int16_t)(bar_w * g->tension / 100);
  if (needle < bar_x) needle = bar_x;
  if (needle > bar_x + bar_w - 3) needle = bar_x + bar_w - 3;
  GColor needle_c = game_in_green() ? C_ACCENT : C_RED;
  graphics_context_set_fill_color(ctx, needle_c);
  graphics_fill_rect(ctx, GRect(needle - 2, bar_y - 4, 5, bar_h + 8), 0, GCornerNone);

  // Reel progress
  int16_t py = bar_y + bar_h + 10;
  graphics_context_set_fill_color(ctx, C_DIM);
  graphics_fill_rect(ctx, GRect(bar_x, py, bar_w, 8), 1, GCornersAll);
  graphics_context_set_fill_color(ctx, C_YELLOW);
  graphics_fill_rect(ctx, GRect(bar_x, py, (int16_t)(bar_w * g->reel_progress / 100), 8),
                     1, GCornersAll);

  graphics_context_set_text_color(ctx, C_TEXT);
  graphics_draw_text(ctx, "Tension", fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(bar_x, bar_y - 18, bar_w, 16),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  graphics_draw_text(ctx, "Reel", fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(bar_x, py + 8, bar_w, 16),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
}

static void canvas_update(Layer *layer, GContext *ctx) {
  const Game *g = game_get();
  GRect bounds = layer_get_bounds(layer);
  char line[40];

  graphics_context_set_fill_color(ctx, C_BG);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  graphics_context_set_text_color(ctx, C_TEXT);

  if (g->state == GameState_Title) {
    // Title scene: draw native size, centered / top-aligned (no upscale tiling)
    if (s_bmp_title) {
      GRect tb = gbitmap_get_bounds(s_bmp_title);
      int16_t tx = (bounds.size.w - tb.size.w) / 2;
      if (tx < 0) tx = 0;
      draw_bitmap_native(ctx, s_bmp_title, GPoint(tx, 0));
      // Fill leftover below the art with water color so it isn't empty black
      if (tb.size.h < bounds.size.h) {
        graphics_context_set_fill_color(ctx, C_WATER);
        graphics_fill_rect(ctx, GRect(0, tb.size.h, bounds.size.w,
                                      bounds.size.h - tb.size.h), 0, GCornerNone);
      }
    } else {
      graphics_draw_text(ctx, "WRIST CAST", fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD),
                         GRect(0, 28, bounds.size.w, 32), GTextOverflowModeTrailingEllipsis,
                         GTextAlignmentCenter, NULL);
    }

    // Bottom caption bar over the art
    int16_t bar_h = (g->total_catches > 0) ? 58 : 46;
    graphics_context_set_fill_color(ctx, C_BG);
    graphics_fill_rect(ctx, GRect(0, bounds.size.h - bar_h, bounds.size.w, bar_h),
                       0, GCornerNone);

    graphics_context_set_text_color(ctx, C_DIM);
    graphics_draw_text(ctx, "Flick to cast · feel the bite",
                       fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(4, bounds.size.h - bar_h + 2, bounds.size.w - 8, 18),
                       GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
    graphics_context_set_text_color(ctx, C_TEXT);
    graphics_draw_text(ctx, "SELECT to start", fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                       GRect(4, bounds.size.h - bar_h + 20, bounds.size.w - 8, 22),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);

    if (g->total_catches > 0) {
      snprintf(line, sizeof(line), "Caught %u", (unsigned)g->total_catches);
      graphics_context_set_text_color(ctx, C_DIM);
      graphics_draw_text(ctx, line, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                         GRect(4, bounds.size.h - 18, bounds.size.w - 8, 16),
                         GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    }
    return;
  }

  if (g->state == GameState_Fighting) {
    graphics_draw_text(ctx, "FIGHT!", fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                       GRect(4, 4, bounds.size.w - 8, 22),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    draw_tension(ctx, bounds, g);
    graphics_context_set_text_color(ctx, C_DIM);
    graphics_draw_text(ctx, "Hold UP reel · DOWN ease", fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(4, bounds.size.h - 28, bounds.size.w - 8, 20),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    return;
  }

  if (g->state == GameState_Caught || g->state == GameState_Snapped) {
    if (g->state == GameState_Caught) {
      GBitmap *fb = fish_bmp_for(g->catch_rarity);
      if (fb) {
        draw_bitmap_centered_native(ctx, fb,
                                    GPoint(bounds.size.w / 2, bounds.size.h / 2 - 24));
      }
      graphics_draw_text(ctx, g->catch_name, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                         GRect(4, bounds.size.h / 2 + 28, bounds.size.w - 8, 22),
                         GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
      graphics_context_set_text_color(ctx, C_DIM);
      graphics_draw_text(ctx, rarity_label(g->catch_rarity),
                         fonts_get_system_font(FONT_KEY_GOTHIC_14),
                         GRect(4, bounds.size.h / 2 + 50, bounds.size.w - 8, 18),
                         GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
      snprintf(line, sizeof(line), "Total %u  Streak %u",
               (unsigned)g->total_catches, (unsigned)g->streak);
      graphics_draw_text(ctx, line, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                         GRect(4, 6, bounds.size.w - 8, 18),
                         GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    } else {
      graphics_draw_text(ctx, "SNAP!", fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD),
                         GRect(4, bounds.size.h / 2 - 30, bounds.size.w - 8, 36),
                         GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
      graphics_context_set_text_color(ctx, C_DIM);
      graphics_draw_text(ctx, "Keep tension in the green",
                         fonts_get_system_font(FONT_KEY_GOTHIC_14),
                         GRect(4, bounds.size.h / 2 + 10, bounds.size.w - 8, 20),
                         GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
    }
    graphics_context_set_text_color(ctx, C_TEXT);
    graphics_draw_text(ctx, "SELECT: again", fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(4, bounds.size.h - 24, bounds.size.w - 8, 18),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    return;
  }

  // Ready / Waiting / Bite
  draw_scene(ctx, bounds, g);

  snprintf(line, sizeof(line), "%u caught", (unsigned)g->total_catches);
  graphics_context_set_text_color(ctx, C_DIM);
  graphics_draw_text(ctx, line, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(4, 2, bounds.size.w - 8, 16),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);

  if (g->state == GameState_Bite) {
    graphics_context_set_text_color(ctx, C_RED);
    graphics_draw_text(ctx, "BITE!", fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD),
                       GRect(4, 18, bounds.size.w - 8, 32),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  }

  if (g->banner[0]) {
    graphics_context_set_text_color(ctx, C_TEXT);
    graphics_draw_text(ctx, g->banner, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                       GRect(4, bounds.size.h - 40, bounds.size.w - 8, 22),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  }

  if (g->state == GameState_Ready) {
    graphics_context_set_text_color(ctx, C_DIM);
    graphics_draw_text(ctx, "or SELECT to cast", fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(4, bounds.size.h - 22, bounds.size.w - 8, 18),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  }
}

static void select_click(ClickRecognizerRef recognizer, void *context) {
  (void)recognizer;
  (void)context;
  game_select();
  ui_mark_dirty();
}

static void up_down(ClickRecognizerRef recognizer, void *context) {
  (void)recognizer;
  (void)context;
  game_set_up_held(true);
}

static void up_up(ClickRecognizerRef recognizer, void *context) {
  (void)recognizer;
  (void)context;
  game_set_up_held(false);
}

static void down_down(ClickRecognizerRef recognizer, void *context) {
  (void)recognizer;
  (void)context;
  game_set_down_held(true);
}

static void down_up(ClickRecognizerRef recognizer, void *context) {
  (void)recognizer;
  (void)context;
  game_set_down_held(false);
}

static void back_click(ClickRecognizerRef recognizer, void *context) {
  (void)recognizer;
  (void)context;
  game_set_up_held(false);
  game_set_down_held(false);
  const Game *g = game_get();
  if (g->state == GameState_Title) {
    window_stack_pop(true);
    return;
  }
  game_back();
  ui_mark_dirty();
}

static void click_config(void *context) {
  (void)context;
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click);
  window_raw_click_subscribe(BUTTON_ID_UP, up_down, up_up, NULL);
  window_raw_click_subscribe(BUTTON_ID_DOWN, down_down, down_up, NULL);
  window_single_click_subscribe(BUTTON_ID_BACK, back_click);
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  s_canvas = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_canvas, canvas_update);
  layer_add_child(root, s_canvas);

  s_bmp_title = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TITLE);
  s_bmp_water = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_WATER);
  s_bmp_rod = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ROD);
  s_bmp_bobber = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BOBBER);
  s_bmp_fish_common = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_FISH_COMMON);
  s_bmp_fish_rare = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_FISH_RARE);
  s_bmp_fish_legend = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_FISH_LEGEND);

  s_redraw = app_timer_register(100, redraw_cb, NULL);
}

static void window_unload(Window *window) {
  (void)window;
  if (s_redraw) {
    app_timer_cancel(s_redraw);
    s_redraw = NULL;
  }
  if (s_bmp_title) { gbitmap_destroy(s_bmp_title); s_bmp_title = NULL; }
  if (s_bmp_water) { gbitmap_destroy(s_bmp_water); s_bmp_water = NULL; }
  if (s_bmp_rod) { gbitmap_destroy(s_bmp_rod); s_bmp_rod = NULL; }
  if (s_bmp_bobber) { gbitmap_destroy(s_bmp_bobber); s_bmp_bobber = NULL; }
  if (s_bmp_fish_common) { gbitmap_destroy(s_bmp_fish_common); s_bmp_fish_common = NULL; }
  if (s_bmp_fish_rare) { gbitmap_destroy(s_bmp_fish_rare); s_bmp_fish_rare = NULL; }
  if (s_bmp_fish_legend) { gbitmap_destroy(s_bmp_fish_legend); s_bmp_fish_legend = NULL; }
  layer_destroy(s_canvas);
  s_canvas = NULL;
}

void ui_init(void) {
  s_window = window_create();
  window_set_background_color(s_window, C_BG);
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load,
    .unload = window_unload,
  });
  window_set_click_config_provider(s_window, click_config);
  window_stack_push(s_window, true);
}

void ui_deinit(void) {
  window_destroy(s_window);
  s_window = NULL;
}

void ui_mark_dirty(void) {
  if (s_canvas) layer_mark_dirty(s_canvas);
}
