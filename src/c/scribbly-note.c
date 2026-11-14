// Scribbly Note — a one-time handwritten note for touchscreen Pebbles.
//
// View mode (default when a note exists): shows the last note. Touch is ignored
//   so you cannot accidentally scribble over your locker number.
//   SELECT  -> start a new note (erases the old one)
//
// Write mode: draw with your finger.
//   UP      -> undo last stroke
//   SELECT  -> done (back to view mode)
//
// The note is saved to persistent storage after every stroke, so it is kept
// even if the app is closed in the middle of writing.

#include <pebble.h>

// ---------------------------------------------------------------------------
// Note storage
//
// A note is a sequence of points (x, y as bytes). A PEN_UP marker separates
// strokes. Points are persisted in chunks of PERSIST_DATA_MAX_LENGTH bytes.

#define PEN_UP 0xFF
#define MAX_POINTS 1500                 // 3000 bytes, within the 4KB app quota
#define CHUNK_BYTES PERSIST_DATA_MAX_LENGTH
#define KEY_COUNT 1                     // number of points stored
#define KEY_CHUNK_BASE 100              // chunks live at keys 100, 101, ...
#define MIN_SEGMENT_SQ 4                // ignore moves shorter than 2px
#define STROKE_WIDTH 5

typedef struct {
  uint8_t x;
  uint8_t y;
} NotePoint;

static NotePoint s_points[MAX_POINTS];
static int s_count;

static Window *s_window;
static Layer *s_canvas;
static TextLayer *s_hint_layer;
static bool s_writing;
static bool s_pen_down;
static bool s_dirty;

static void prv_save(void) {
  if (!s_dirty) {
    return;
  }
  const uint8_t *bytes = (const uint8_t *)s_points;
  const int total = s_count * (int)sizeof(NotePoint);
  int chunk = 0;
  for (int offset = 0; offset < total; offset += CHUNK_BYTES, chunk++) {
    int len = total - offset;
    if (len > CHUNK_BYTES) {
      len = CHUNK_BYTES;
    }
    persist_write_data(KEY_CHUNK_BASE + chunk, bytes + offset, len);
  }
  // Remove chunks left over from a longer previous note.
  while (persist_exists(KEY_CHUNK_BASE + chunk)) {
    persist_delete(KEY_CHUNK_BASE + chunk);
    chunk++;
  }
  persist_write_int(KEY_COUNT, s_count);
  s_dirty = false;
}

static void prv_load(void) {
  s_count = 0;
  if (!persist_exists(KEY_COUNT)) {
    return;
  }
  int count = persist_read_int(KEY_COUNT);
  if (count < 0 || count > MAX_POINTS) {
    return;
  }
  uint8_t *bytes = (uint8_t *)s_points;
  const int total = count * (int)sizeof(NotePoint);
  int chunk = 0;
  for (int offset = 0; offset < total; offset += CHUNK_BYTES, chunk++) {
    int len = total - offset;
    if (len > CHUNK_BYTES) {
      len = CHUNK_BYTES;
    }
    if (persist_read_data(KEY_CHUNK_BASE + chunk, bytes + offset, len) != len) {
      return;  // corrupt / incomplete: treat as no note
    }
  }
  s_count = count;
}

// ---------------------------------------------------------------------------
// Note editing

static bool prv_is_pen_up(int i) {
  return s_points[i].x == PEN_UP;
}

static void prv_add_point(int16_t x, int16_t y) {
  if (s_count >= MAX_POINTS - 1) {  // keep room for the closing PEN_UP
    return;
  }
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  if (x > 254) x = 254;
  if (y > 254) y = 254;
  s_points[s_count++] = (NotePoint){ .x = (uint8_t)x, .y = (uint8_t)y };
  s_dirty = true;
}

static void prv_end_stroke(void) {
  if (s_count > 0 && !prv_is_pen_up(s_count - 1)) {
    s_points[s_count++] = (NotePoint){ .x = PEN_UP, .y = PEN_UP };
    s_dirty = true;
  }
}

static void prv_undo_stroke(void) {
  if (s_count == 0) {
    return;
  }
  // Drop the trailing PEN_UP, then all points back to the previous PEN_UP.
  if (prv_is_pen_up(s_count - 1)) {
    s_count--;
  }
  while (s_count > 0 && !prv_is_pen_up(s_count - 1)) {
    s_count--;
  }
  s_dirty = true;
}

// ---------------------------------------------------------------------------
// UI

static void prv_update_hint(void) {
  const char *hint = NULL;
  if (!touch_service_is_enabled()) {
    hint = "Touch screen\nnot available";
  } else if (s_writing && s_count == 0) {
    hint = "Write your note\n\nUP: undo\nSELECT: done";
  } else if (!s_writing && s_count == 0) {
    hint = "No note yet\n\nSELECT: new note";
  }
  layer_set_hidden(text_layer_get_layer(s_hint_layer), hint == NULL);
  if (hint) {
    text_layer_set_text(s_hint_layer, hint);
  }
}

static void prv_canvas_update(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, STROKE_WIDTH);

  for (int i = 0; i < s_count; i++) {
    if (prv_is_pen_up(i)) {
      continue;
    }
    GPoint p = GPoint(s_points[i].x, s_points[i].y);
    bool has_prev = i > 0 && !prv_is_pen_up(i - 1);
    bool has_next = i + 1 < s_count && !prv_is_pen_up(i + 1);
    if (has_prev) {
      graphics_draw_line(ctx, GPoint(s_points[i - 1].x, s_points[i - 1].y), p);
    } else if (!has_next) {
      graphics_fill_circle(ctx, p, STROKE_WIDTH / 2);  // a single dot
    }
  }

  // Thin frame in write mode so it is clear the screen is "live".
  if (s_writing) {
    graphics_context_set_stroke_width(ctx, 1);
    graphics_context_set_stroke_color(ctx, GColorLightGray);
    graphics_draw_rect(ctx, grect_inset(bounds, GEdgeInsets(2)));
  }
}

static void prv_set_writing(bool writing) {
  s_writing = writing;
  s_pen_down = false;
  prv_update_hint();
  layer_mark_dirty(s_canvas);
}

static void prv_touch_handler(const TouchEvent *event, void *context) {
  if (!s_writing) {
    return;
  }
  switch (event->type) {
    case TouchEvent_Touchdown:
      prv_end_stroke();  // safety: never join onto a previous stroke
      prv_add_point(event->x, event->y);
      s_pen_down = true;
      prv_update_hint();
      break;
    case TouchEvent_PositionUpdate:
      if (!s_pen_down || s_count == 0) {
        break;
      }
      {
        NotePoint last = s_points[s_count - 1];
        int dx = event->x - last.x;
        int dy = event->y - last.y;
        if (dx * dx + dy * dy < MIN_SEGMENT_SQ) {
          return;
        }
      }
      prv_add_point(event->x, event->y);
      break;
    case TouchEvent_Liftoff:
      if (s_pen_down) {
        prv_end_stroke();
        s_pen_down = false;
        prv_save();
      }
      break;
  }
  layer_mark_dirty(s_canvas);
}

static void prv_select_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_writing) {
    prv_end_stroke();
    prv_save();
    prv_set_writing(false);
  } else {
    // New note: the old one is gone.
    s_count = 0;
    s_dirty = true;
    prv_save();
    prv_set_writing(true);
  }
}

static void prv_up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (!s_writing) {
    return;
  }
  s_pen_down = false;
  prv_undo_stroke();
  prv_save();
  prv_update_hint();
  layer_mark_dirty(s_canvas);
}

static void prv_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, prv_select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, prv_up_click_handler);
}

static void prv_window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);

  s_canvas = layer_create(bounds);
  layer_set_update_proc(s_canvas, prv_canvas_update);
  layer_add_child(root, s_canvas);

  s_hint_layer = text_layer_create(GRect(8, bounds.size.h / 2 - 50, bounds.size.w - 16, 110));
  text_layer_set_background_color(s_hint_layer, GColorClear);
  text_layer_set_text_color(s_hint_layer, GColorDarkGray);
  text_layer_set_font(s_hint_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_hint_layer, GTextAlignmentCenter);
  layer_add_child(root, text_layer_get_layer(s_hint_layer));

  // Start writing straight away if there is nothing to show.
  prv_set_writing(s_count == 0);
}

static void prv_window_unload(Window *window) {
  text_layer_destroy(s_hint_layer);
  layer_destroy(s_canvas);
}

static void prv_init(void) {
  prv_load();

  s_window = window_create();
  window_set_background_color(s_window, GColorWhite);
  // Take over the touch screen instead of the system's button emulation.
  window_set_touch_bridge_disabled(s_window, true);
  window_set_click_config_provider(s_window, prv_click_config_provider);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  window_stack_push(s_window, true);

  touch_service_subscribe(prv_touch_handler, NULL);
}

static void prv_deinit(void) {
  touch_service_unsubscribe();
  prv_end_stroke();
  prv_save();
  window_destroy(s_window);
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
