#include <pebble.h>

static Window *s_window;
static SimpleMenuLayer *s_simple_menu_layer;
static SimpleMenuSection s_menu_sections[2];
static SimpleMenuItem s_sound_items[9];
static SimpleMenuItem s_volume_items[3];
static char s_volume_subtitle[16];
static int s_last_index = -1;
static AppTimer *s_volume_timer = NULL;
static int s_volume_steps_remaining = 0;
static void (*s_volume_done_cb)(void) = NULL;
static AppTimer *s_alt_loop_timer = NULL;
static bool s_alt_loop_toggle = false;

// --- Volume max logic ---

static void volume_up_timer_callback(void *context) {
  s_volume_timer = NULL;
  if (s_volume_steps_remaining <= 0) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Volume at max");
    if (s_volume_done_cb) {
      s_volume_done_cb();
      s_volume_done_cb = NULL;
    }
    return;
  }
  music_volume_up();
  s_volume_steps_remaining--;
  s_volume_timer = app_timer_register(100, volume_up_timer_callback, NULL);
}

static void set_volume_max(void) {
  // Cancel any running sequence
  if (s_volume_timer) {
    app_timer_cancel(s_volume_timer);
    s_volume_timer = NULL;
  }

  uint8_t current = music_get_volume_percent();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Current volume: %d%%", current);

  // 16 steps = 100%, each step = 6.25% = 25/4%
  // steps = ceil((100 - current) * 4 / 25)
  int needed = current >= 100 ? 0 : ((100 - current) * 4 + 24) / 25;
  s_volume_steps_remaining = needed > 16 ? 16 : needed;

  if (s_volume_steps_remaining == 0) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Volume already at max");
    return;
  }
  s_volume_timer = app_timer_register(100, volume_up_timer_callback, NULL);
}

static void set_volume_max_with_callback(void (*cb)(void)) {
  if (s_volume_timer) {
    app_timer_cancel(s_volume_timer);
    s_volume_timer = NULL;
  }

  uint8_t current = music_get_volume_percent();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Current volume: %d%%", current);

  int needed = current >= 100 ? 0 : ((100 - current) * 4 + 24) / 25;
  s_volume_steps_remaining = needed > 16 ? 16 : needed;
  s_volume_done_cb = cb;

  if (s_volume_steps_remaining == 0) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Volume already at max");
    if (s_volume_done_cb) {
      s_volume_done_cb();
      s_volume_done_cb = NULL;
    }
    return;
  }
  s_volume_timer = app_timer_register(100, volume_up_timer_callback, NULL);
}

static void play_alarm(void) {
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  dict_write_cstring(iter, MESSAGE_KEY_PLAY_SOUND, "alarm");
  app_message_outbox_send();
}

static void alarm_maxvol_callback(int index, void *ctx) {
  set_volume_max_with_callback(play_alarm);
}

static void alt_loop_send(void *context) {
  s_alt_loop_timer = NULL;
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  dict_write_cstring(iter, MESSAGE_KEY_PLAY_SOUND, s_alt_loop_toggle ? "alarm" : "voice");
  app_message_outbox_send();
  s_alt_loop_toggle = !s_alt_loop_toggle;
  s_alt_loop_timer = app_timer_register(1500, alt_loop_send, NULL);
}

static void stop_alt_loop(void) {
  if (s_alt_loop_timer) {
    app_timer_cancel(s_alt_loop_timer);
    s_alt_loop_timer = NULL;
  }
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  dict_write_uint8(iter, MESSAGE_KEY_STOP_SOUND, 1);
  app_message_outbox_send();
}

static void start_alt_loop(void) {
  s_alt_loop_toggle = false;
  s_alt_loop_timer = app_timer_register(100, alt_loop_send, NULL);
}

static void voice_alarm_maxvol_callback(int index, void *ctx) {
  if (index == s_last_index) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Stopping voice+alarm loop");
    stop_alt_loop();
    s_last_index = -1;
    return;
  }
  stop_alt_loop();
  s_last_index = index;
  set_volume_max_with_callback(start_alt_loop);
}

// --- Sound menu callbacks ---

static void sound_menu_select_callback(int index, void *ctx) {
  bool is_loop = (index == 1 || index == 3 || index == 5);

  if (is_loop && index == s_last_index) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Stopping loop for index %d", index);
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    dict_write_uint8(iter, MESSAGE_KEY_STOP_SOUND, 1);
    app_message_outbox_send();
    s_last_index = -1;
    return;
  }

  char type[20];
  switch (index) {
    case 0: snprintf(type, sizeof(type), "voice"); break;
    case 1: snprintf(type, sizeof(type), "voice_loop"); break;
    case 2: snprintf(type, sizeof(type), "alarm"); break;
    case 3: snprintf(type, sizeof(type), "alarm_loop"); break;
    case 4: snprintf(type, sizeof(type), "sms"); break;
    case 5: snprintf(type, sizeof(type), "sms_loop"); break;
    case 6: {
        DictionaryIterator *iter;
        app_message_outbox_begin(&iter);
        dict_write_uint8(iter, MESSAGE_KEY_STOP_SOUND, 1);
        app_message_outbox_send();
        s_last_index = -1;
        return;
      }
    default: snprintf(type, sizeof(type), "voice"); break;
  }

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Sending play type: %s", type);
  s_last_index = index;

  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  dict_write_cstring(iter, MESSAGE_KEY_PLAY_SOUND, type);
  app_message_outbox_send();
}

// --- Volume menu callback ---

static void volume_menu_select_callback(int index, void *ctx) {
  if (index == 0) {
    // Vol Max: use firmware music_volume_up() directly
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Setting volume to max via firmware");
    set_volume_max();
  } else if (index == 1) {
    // Vol Down once
    music_volume_down();
  } else if (index == 2) {
    // Show current volume value
    uint8_t current = music_get_volume_percent();
    snprintf(s_volume_subtitle, sizeof(s_volume_subtitle), "%d%%", current);
    s_volume_items[2].subtitle = s_volume_subtitle;
    menu_layer_reload_data(simple_menu_layer_get_menu_layer(s_simple_menu_layer));
  }
}

// --- Inbox ---

static void prv_inbox_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *success_tuple = dict_find(iter, MESSAGE_KEY_PLAY_SUCCESS);
  Tuple *error_tuple = dict_find(iter, MESSAGE_KEY_PLAY_ERROR);

  if (error_tuple) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Playback error");
    vibes_short_pulse();
    s_last_index = -1;
  } else if (success_tuple) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Action successful!");
  }
}

// --- Window load/unload ---

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Sound section
  s_sound_items[0] = (SimpleMenuItem) { .title = "Voice", .callback = sound_menu_select_callback };
  s_sound_items[1] = (SimpleMenuItem) { .title = "Voice Loop", .callback = sound_menu_select_callback };
  s_sound_items[2] = (SimpleMenuItem) { .title = "Alarm", .callback = sound_menu_select_callback };
  s_sound_items[3] = (SimpleMenuItem) { .title = "Alarm Loop", .callback = sound_menu_select_callback };
  s_sound_items[4] = (SimpleMenuItem) { .title = "SMS", .callback = sound_menu_select_callback };
  s_sound_items[5] = (SimpleMenuItem) { .title = "SMS Loop", .callback = sound_menu_select_callback };
  s_sound_items[6] = (SimpleMenuItem) { .title = "Stop All", .callback = sound_menu_select_callback };
  s_sound_items[7] = (SimpleMenuItem) { .title = "Alarm + Vol Max", .callback = alarm_maxvol_callback };
  s_sound_items[8] = (SimpleMenuItem) { .title = "Voice+Alarm+Max Lp", .callback = voice_alarm_maxvol_callback };

  s_menu_sections[0] = (SimpleMenuSection) {
    .title = "Sound",
    .num_items = 9,
    .items = s_sound_items,
  };

  // Volume section
  s_volume_items[0] = (SimpleMenuItem) { .title = "Vol Max", .callback = volume_menu_select_callback };
  s_volume_items[1] = (SimpleMenuItem) { .title = "Vol Down", .callback = volume_menu_select_callback };
  s_volume_items[2] = (SimpleMenuItem) { .title = "Vol Value", .subtitle = "\x00", .callback = volume_menu_select_callback };

  s_menu_sections[1] = (SimpleMenuSection) {
    .title = "Volume",
    .num_items = 3,
    .items = s_volume_items,
  };

  s_simple_menu_layer = simple_menu_layer_create(bounds, window, s_menu_sections, 2, NULL);
  layer_add_child(window_layer, simple_menu_layer_get_layer(s_simple_menu_layer));
}

static void prv_window_unload(Window *window) {
  if (s_volume_timer) {
    app_timer_cancel(s_volume_timer);
    s_volume_timer = NULL;
  }
  stop_alt_loop();
  simple_menu_layer_destroy(s_simple_menu_layer);
}

static void prv_init(void) {
  app_message_register_inbox_received(prv_inbox_received_handler);
  app_message_open(128, 128);
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  window_stack_push(s_window, true);
}

static void prv_deinit(void) {
  window_destroy(s_window);
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
