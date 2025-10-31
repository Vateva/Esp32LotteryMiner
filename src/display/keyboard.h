// keyboard.h
#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <Arduino.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#include "config.h"

// active keyboard layout state
enum keyboard_mode_t { KB_LOWERCASE, KB_UPPERCASE, KB_NUMBERS };

// stores properties for a single on-screen key
struct keyboard_key_t {
  int16_t x, y;
  uint16_t w, h;
  char normal_char;
  char shift_char;
  bool is_special;
  uint8_t special_id;
};

// non-character key identifiers
#define KEY_SHIFT 1
#define KEY_BACKSPACE 2
#define KEY_SPACE 3
#define KEY_RETURN 4
#define KEY_MODE_SWITCH 5

class Keyboard {
 private:
  // label text
  static const uint8_t MAX_LABEL_LENGTH = 63;
  char label_text[MAX_LABEL_LENGTH + 1];

  // layout dimensions from config.h
  static const uint16_t INPUT_AREA_HEIGHT = KB_INPUT_AREA_HEIGHT;
  static const uint16_t KEYBOARD_Y_START = KB_KEYBOARD_Y_START;
  static const uint16_t KEYBOARD_HEIGHT = KB_KEYBOARD_HEIGHT;
  static const uint16_t KEY_SPACING = KB_KEY_SPACING;

  // input buffer state
  static const uint8_t MAX_BUFFER_SIZE = 128;
  char input_buffer[MAX_BUFFER_SIZE];
  uint8_t buffer_length;
  uint8_t max_length;
  bool requires_redraw;

  // keyboard mode state
  keyboard_mode_t current_mode;
  bool input_complete;
  bool mode_changed;
  bool needs_full_clear;  // tracks if full screen clear is needed

  // key press highlight state
  int8_t pressed_key_index;
  unsigned long press_timestamp;
  static const uint16_t PRESS_HIGHLIGHT_DURATION = KB_PRESS_HIGHLIGHT_MS;

  // debounce
  unsigned long last_touch_time;
  static const uint16_t DEBOUNCE_DELAY = 200;  // milliseconds

  // layout definitions
  keyboard_key_t lowercase_keys[MAX_KEYS];
  keyboard_key_t uppercase_keys[MAX_KEYS];
  keyboard_key_t number_keys[MAX_KEYS];
  uint8_t num_keys;

  // private methods
  void init_lowercase_layout();
  void init_uppercase_layout();
  void init_number_layout();
  void draw_key(lgfx::LGFX_Device* lcd, const keyboard_key_t& key, bool highlighted);
  void draw_input_area(lgfx::LGFX_Device* lcd);
  int8_t find_touched_key(uint16_t tx, uint16_t ty);
  void handle_key_press(int8_t key_index);
  void add_character(char c);

 public:
  Keyboard();

  void set_max_length(uint8_t len);
  void clear();

  void draw(lgfx::LGFX_Device* lcd);
  void mark_for_redraw();
  ;
  void handle_touch(uint16_t tx, uint16_t ty, lgfx::LGFX_Device* lcd);
  void show(lgfx::LGFX_Device* lcd);

  bool is_complete() const;
  const char* get_text() const;
  void reset_complete();

  void set_label(const char* text);
};

#endif