// keyboard.cpp
#include "keyboard.h"

Keyboard::Keyboard() {
  buffer_length = 0;
  input_buffer[0] = '\0';
  max_length = MAX_BUFFER_SIZE - 1;
  current_mode = KB_LOWERCASE;
  input_complete = false;
  pressed_key_index = -1;
  press_timestamp = 0;
  requires_redraw = true;
  last_touch_time = 0;
  mode_changed = false;
  needs_full_clear = true;
  label_text[0] = '\0';  // no label by default

  // initialize all three key layouts
  init_lowercase_layout();
  init_uppercase_layout();
  init_number_layout();
}

void Keyboard::mark_for_redraw() {
  requires_redraw = true;
}

void Keyboard::set_max_length(uint8_t len) {
  if (len < MAX_BUFFER_SIZE) {
    max_length = len;
  }
}

void Keyboard::clear() {
    buffer_length = 0;
    input_buffer[0] = '\0';
    input_complete = false;
    pressed_key_index = -1;
    needs_full_clear = true;
    label_text[0] = '\0';  // clear label too
    mark_for_redraw();
}

// sets the label text to display above input area
void Keyboard::set_label(const char* text) {
  if (text != nullptr) {
    strncpy(label_text, text, MAX_LABEL_LENGTH);
    label_text[MAX_LABEL_LENGTH] = '\0';
  } else {
    label_text[0] = '\0';
  }
  mark_for_redraw();
}



void Keyboard::reset_complete() {
  input_complete = false;
}

bool Keyboard::is_complete() const {
  return input_complete;
}

const char* Keyboard::get_text() const {
  return input_buffer;
}

// defines geometry and mapping for lowercase layout
void Keyboard::init_lowercase_layout() {
  num_keys = 0;

  // row 1: "qwertyuiop"
  const char row1[] = "qwertyuiop";
  int16_t x = KEY_SPACING;
  int16_t y = KEYBOARD_Y_START + KEY_SPACING;
  for (uint8_t i = 0; i < 10; i++) {
    lowercase_keys[num_keys].x = x;
    lowercase_keys[num_keys].y = y;
    lowercase_keys[num_keys].w = 30;
    lowercase_keys[num_keys].h = 42;
    lowercase_keys[num_keys].normal_char = row1[i];
    lowercase_keys[num_keys].shift_char = row1[i];
    lowercase_keys[num_keys].is_special = false;
    num_keys++;
    x += 30 + KEY_SPACING;
  }

  // row 2: "asdfghjkl" (15px offset)
  const char row2[] = "asdfghjkl";
  x = 15 + KEY_SPACING;
  y += 42 + KEY_SPACING;
  for (uint8_t i = 0; i < 9; i++) {
    lowercase_keys[num_keys].x = x;
    lowercase_keys[num_keys].y = y;
    lowercase_keys[num_keys].w = 30;
    lowercase_keys[num_keys].h = 42;
    lowercase_keys[num_keys].normal_char = row2[i];
    lowercase_keys[num_keys].shift_char = row2[i];
    lowercase_keys[num_keys].is_special = false;
    num_keys++;
    x += 30 + KEY_SPACING;
  }

  // row 3: shift "zxcvbnm" backspace
  y += 42 + KEY_SPACING;
  x = KEY_SPACING;

  // shift key
  lowercase_keys[num_keys].x = x;
  lowercase_keys[num_keys].y = y;
  lowercase_keys[num_keys].w = 42;
  lowercase_keys[num_keys].h = 42;
  lowercase_keys[num_keys].is_special = true;
  lowercase_keys[num_keys].special_id = KEY_SHIFT;
  num_keys++;
  x += 42 + KEY_SPACING;

  // character keys 'z' through 'm'
  const char row3[] = "zxcvbnm";
  for (uint8_t i = 0; i < 7; i++) {
    lowercase_keys[num_keys].x = x;
    lowercase_keys[num_keys].y = y;
    lowercase_keys[num_keys].w = 30;
    lowercase_keys[num_keys].h = 42;
    lowercase_keys[num_keys].normal_char = row3[i];
    lowercase_keys[num_keys].shift_char = row3[i];
    lowercase_keys[num_keys].is_special = false;
    num_keys++;
    x += 30 + KEY_SPACING;
  }

  // backspace key
  lowercase_keys[num_keys].x = x;
  lowercase_keys[num_keys].y = y;
  lowercase_keys[num_keys].w = 42;
  lowercase_keys[num_keys].h = 42;
  lowercase_keys[num_keys].is_special = true;
  lowercase_keys[num_keys].special_id = KEY_BACKSPACE;
  num_keys++;

  // row 4: mode switch space return
  y += 42 + KEY_SPACING;
  x = KEY_SPACING;

  // mode switch key ("123")
  lowercase_keys[num_keys].x = x;
  lowercase_keys[num_keys].y = y;
  lowercase_keys[num_keys].w = 55;
  lowercase_keys[num_keys].h = 42;
  lowercase_keys[num_keys].is_special = true;
  lowercase_keys[num_keys].special_id = KEY_MODE_SWITCH;
  num_keys++;
  x += 55 + KEY_SPACING;

  // space key
  lowercase_keys[num_keys].x = x;
  lowercase_keys[num_keys].y = y;
  lowercase_keys[num_keys].w = 140;
  lowercase_keys[num_keys].h = 42;
  lowercase_keys[num_keys].is_special = true;
  lowercase_keys[num_keys].special_id = KEY_SPACE;
  num_keys++;
  x += 140 + KEY_SPACING;

  // return key
  lowercase_keys[num_keys].x = x;
  lowercase_keys[num_keys].y = y;
  lowercase_keys[num_keys].w = 55;
  lowercase_keys[num_keys].h = 42;
  lowercase_keys[num_keys].is_special = true;
  lowercase_keys[num_keys].special_id = KEY_RETURN;
  num_keys++;
}

// init uppercase layout (copies and modifies lowercase)
void Keyboard::init_uppercase_layout() {
  // copy layout structure from lowercase_keys
  memcpy(uppercase_keys, lowercase_keys, sizeof(lowercase_keys));

  // convert character keys to uppercase
  for (uint8_t i = 0; i < num_keys; i++) {
    if (!uppercase_keys[i].is_special && uppercase_keys[i].normal_char >= 'a' && uppercase_keys[i].normal_char <= 'z') {
      uppercase_keys[i].normal_char = uppercase_keys[i].normal_char - 32;  // 'a' -> 'A'
      uppercase_keys[i].shift_char = uppercase_keys[i].normal_char;
    }
  }
}

// defines geometry and mapping for numbers/symbols layout
void Keyboard::init_number_layout() {
  uint8_t idx = 0;

  // row 1: "1234567890"
  const char row1[] = "1234567890";
  int16_t x = KEY_SPACING;
  int16_t y = KEYBOARD_Y_START + KEY_SPACING;
  for (uint8_t i = 0; i < 10; i++) {
    number_keys[idx].x = x;
    number_keys[idx].y = y;
    number_keys[idx].w = 30;
    number_keys[idx].h = 42;
    number_keys[idx].normal_char = row1[i];
    number_keys[idx].shift_char = row1[i];
    number_keys[idx].is_special = false;
    idx++;
    x += 30 + KEY_SPACING;
  }

  // row 2: "@.-_:/?!#" (15px offset)
  const char row2[] = "@.-_:/?!#";
  x = 15 + KEY_SPACING;
  y += 42 + KEY_SPACING;
  for (uint8_t i = 0; i < 9; i++) {
    number_keys[idx].x = x;
    number_keys[idx].y = y;
    number_keys[idx].w = 30;
    number_keys[idx].h = 42;
    number_keys[idx].normal_char = row2[i];
    number_keys[idx].shift_char = row2[i];
    number_keys[idx].is_special = false;
    idx++;
    x += 30 + KEY_SPACING;
  }

  // row 3: "$()&*\";'" backspace
  y += 42 + KEY_SPACING;
  x = KEY_SPACING + 21;  // offset to center row 3

  const char row3[] = "$()&*\";'";
  for (uint8_t i = 0; i < 8; i++) {
    number_keys[idx].x = x;
    number_keys[idx].y = y;
    number_keys[idx].w = 30;
    number_keys[idx].h = 42;
    number_keys[idx].normal_char = row3[i];
    number_keys[idx].shift_char = row3[i];
    number_keys[idx].is_special = false;
    idx++;
    x += 30 + KEY_SPACING;
  }

  // backspace key
  number_keys[idx].x = x;
  number_keys[idx].y = y;
  number_keys[idx].w = 42;
  number_keys[idx].h = 42;
  number_keys[idx].is_special = true;
  number_keys[idx].special_id = KEY_BACKSPACE;
  idx++;

  // row 4: mode switch space return
  y += 42 + KEY_SPACING;
  x = KEY_SPACING;

  // mode switch key ("abc")
  number_keys[idx].x = x;
  number_keys[idx].y = y;
  number_keys[idx].w = 55;
  number_keys[idx].h = 42;
  number_keys[idx].is_special = true;
  number_keys[idx].special_id = KEY_MODE_SWITCH;
  idx++;
  x += 55 + KEY_SPACING;

  // space key
  number_keys[idx].x = x;
  number_keys[idx].y = y;
  number_keys[idx].w = 140;
  number_keys[idx].h = 42;
  number_keys[idx].is_special = true;
  number_keys[idx].special_id = KEY_SPACE;
  idx++;
  x += 140 + KEY_SPACING;

  // return key
  number_keys[idx].x = x;
  number_keys[idx].y = y;
  number_keys[idx].w = 55;
  number_keys[idx].h = 42;
  number_keys[idx].is_special = true;
  number_keys[idx].special_id = KEY_RETURN;
  idx++;
}

// renders a single key
void Keyboard::draw_key(lgfx::LGFX_Device* lcd, const keyboard_key_t& key, bool highlighted) {
  // select colors based on highlight state
  uint32_t bg_color = highlighted ? KB_KEY_PRESSED_COLOR : KB_KEY_NORMAL_COLOR;
  uint32_t border_color = COLOR_BLACK;
  uint32_t text_color = KB_KEY_TEXT_COLOR;

  // draw key shape
  lcd->fillRect(key.x, key.y, key.w, key.h, bg_color);
  lcd->drawRect(key.x, key.y, key.w, key.h, border_color);

  // draw key text/icon
  lcd->setTextColor(text_color);
  lcd->setTextSize(2);

  if (key.is_special) {
    // draw special key icons
    lcd->setTextSize(1);
    switch (key.special_id) {
      case KEY_SHIFT:
        lcd->setCursor(key.x + 12, key.y + 16);
        lcd->print("^");
        break;
      case KEY_BACKSPACE:
        lcd->setCursor(key.x + 12, key.y + 16);
        lcd->print("<");
        break;
      case KEY_SPACE:
        lcd->setCursor(key.x + 50, key.y + 16);
        lcd->print("space");
        break;
      case KEY_RETURN:
        lcd->setCursor(key.x + 15, key.y + 16);
        lcd->print("ret");
        break;
      case KEY_MODE_SWITCH:
        lcd->setCursor(key.x + 10, key.y + 16);
        if (current_mode == KB_NUMBERS) {
          lcd->print("abc");
        } else {
          lcd->print("123");
        }
        break;
    }
  } else {
    // draw standard character
    char display_char = (current_mode == KB_UPPERCASE) ? key.shift_char : key.normal_char;
    lcd->setCursor(key.x + 10, key.y + 12);
    lcd->write(display_char);
  }
}

// renders the top input text area
void Keyboard::draw_input_area(lgfx::LGFX_Device* lcd) {
    // clear input area background
    lcd->fillRect(0, 0, SCREEN_WIDTH, INPUT_AREA_HEIGHT, KB_BG_COLOR);
    lcd->drawRect(0, 0, SCREEN_WIDTH, INPUT_AREA_HEIGHT, KB_BORDER_COLOR);
    
    // draw label if present (small text at top, centered)
    bool has_label = (label_text[0] != '\0');
    if (has_label) {
        lcd->setTextSize(1);  // small text size
        lcd->setTextColor(KB_TEXT_COLOR);
        
        // calculate centered position
        // each character in size 1 is approximately 6 pixels wide
        uint8_t label_len = strlen(label_text);
        uint16_t text_width = label_len * 6;
        uint16_t x_centered = (SCREEN_WIDTH - text_width) / 2;
        
        lcd->setCursor(x_centered, 5);  // 5 pixels from top
        lcd->print(label_text);
    }
    
    // adjust input text position based on whether label exists
    uint16_t input_y_offset = has_label ? 28 : 25;  // shift down if label present
    
    if (buffer_length <= KB_TEXT_THRESHOLD) {
        // short text: use large font single line
        lcd->setTextSize(KB_TEXT_SIZE_LARGE);
        lcd->setTextColor(KB_TEXT_COLOR);
        lcd->setCursor(5, input_y_offset);
        
        // print buffer chars
        for (uint8_t i = 0; i < buffer_length; i++) {
            lcd->write(input_buffer[i]);
        }
        
        // draw cursor
        lcd->write('_');
    } else {
        // long text: use small font manual wrap
        lcd->setTextSize(KB_TEXT_SIZE_SMALL);
        lcd->setTextColor(KB_TEXT_COLOR);
        
        const uint8_t chars_per_line = 52;  // chars per line (approx)
        const uint8_t line_height = 12;     // text height + spacing
        const uint8_t x_margin = 5;
        const uint8_t y_start = has_label ? 18 : 8;  // adjust start position if label present
        
        uint8_t current_line = 0;
        uint8_t chars_on_line = 0;
        
        lcd->setCursor(x_margin, y_start);
        
        // print buffer with manual wrap
        for (uint8_t i = 0; i < buffer_length; i++) {
            // check for line wrap
            if (chars_on_line >= chars_per_line) {
                current_line++;
                chars_on_line = 0;
                lcd->setCursor(x_margin, y_start + (current_line * line_height));
            }
            
            lcd->write(input_buffer[i]);
            chars_on_line++;
        }
        
        // draw cursor handling wrap
        if (chars_on_line >= chars_per_line) {
            current_line++;
            lcd->setCursor(x_margin, y_start + (current_line * line_height));
        }
        lcd->write('_');
    }
}

// main draw call (updates screen)
void Keyboard::draw(lgfx::LGFX_Device* lcd) {
  // check if full screen clear is needed (first show or after clear())
  if (needs_full_clear) {
    lcd->fillScreen(KB_BG_COLOR);  // clear entire screen
    needs_full_clear = false;      // only clear once
  }

  // check if highlight just expired
  bool highlight_just_expired = false;
  if (pressed_key_index >= 0) {
    unsigned long time_since_press = millis() - press_timestamp;
    if (time_since_press >= PRESS_HIGHLIGHT_DURATION && time_since_press < PRESS_HIGHLIGHT_DURATION + 50) {
      highlight_just_expired = true;
    }
  }

  // clear keyboard area if layout changed (leaves input area intact)
  if (mode_changed) {
    // calculate keyboard area height: from keyboard start to screen bottom
    uint16_t keyboard_area_height = SCREEN_HEIGHT - KEYBOARD_Y_START;
    // clear only the keyboard area, not the input box at top
    lcd->fillRect(0, KEYBOARD_Y_START, SCREEN_WIDTH, keyboard_area_height, KB_BG_COLOR);
    mode_changed = false;  // reset flag after clearing
  }

  // only redraw if state changed or highlight expired
  if (!requires_redraw && !highlight_just_expired) {
    return;
  }

  // redraw input area
  draw_input_area(lcd);

  // select active key layout
  keyboard_key_t* current_keys;
  if (current_mode == KB_LOWERCASE) {
    current_keys = lowercase_keys;
  } else if (current_mode == KB_UPPERCASE) {
    current_keys = uppercase_keys;
  } else {
    current_keys = number_keys;
  }

  // check if highlight is still active
  bool highlight_active = false;
  if (pressed_key_index >= 0 && (millis() - press_timestamp) < PRESS_HIGHLIGHT_DURATION) {
    highlight_active = true;
  }

  // draw all keys in the layout
  for (uint8_t i = 0; i < num_keys; i++) {
    bool highlighted = (highlight_active && i == pressed_key_index);
    draw_key(lcd, current_keys[i], highlighted);
  }
  requires_redraw = false;  // reset redraw flag
}

// finds key index from (tx ty) coords
int8_t Keyboard::find_touched_key(uint16_t tx, uint16_t ty) {
  // get active key layout
  keyboard_key_t* current_keys;
  if (current_mode == KB_LOWERCASE) {
    current_keys = lowercase_keys;
  } else if (current_mode == KB_UPPERCASE) {
    current_keys = uppercase_keys;
  } else {
    current_keys = number_keys;
  }

  // iterate keys and check bounds
  for (uint8_t i = 0; i < num_keys; i++) {
    if (tx >= current_keys[i].x && tx <= (current_keys[i].x + current_keys[i].w) && ty >= current_keys[i].y &&
        ty <= (current_keys[i].y + current_keys[i].h)) {
      return i;
    }
  }

  return -1;  // no key found
}

// processes a key press event by index
void Keyboard::handle_key_press(int8_t key_index) {
  if (key_index < 0) return;

  // get active key layout
  keyboard_key_t* current_keys;
  if (current_mode == KB_LOWERCASE) {
    current_keys = lowercase_keys;
  } else if (current_mode == KB_UPPERCASE) {
    current_keys = uppercase_keys;
  } else {
    current_keys = number_keys;
  }

  const keyboard_key_t& key = current_keys[key_index];

  if (key.is_special) {
    // process special keys (shift mode etc)
    switch (key.special_id) {
      case KEY_SHIFT:
        // toggle lower/upper case
        if (current_mode == KB_LOWERCASE) {
          current_mode = KB_UPPERCASE;
          mode_changed = true;  // flag that layout changed
        } else if (current_mode == KB_UPPERCASE) {
          current_mode = KB_LOWERCASE;
          mode_changed = true;  // flag that layout changed
        }
        break;

      case KEY_BACKSPACE:
        // remove last char
        if (buffer_length > 0) {
          buffer_length--;
          input_buffer[buffer_length] = '\0';
        }
        break;

      case KEY_SPACE:
        // add space
        add_character(' ');
        break;

      case KEY_RETURN:
        // flag input as complete
        input_complete = true;
        break;

      case KEY_MODE_SWITCH:
        // toggle letters/numbers mode
        if (current_mode == KB_NUMBERS) {
          current_mode = KB_LOWERCASE;
          mode_changed = true;  // flag that layout changed
        } else {
          current_mode = KB_NUMBERS;
          mode_changed = true;  // flag that layout changed
        }
        break;
    }
  } else {
    // process standard character
    char c = (current_mode == KB_UPPERCASE) ? key.shift_char : key.normal_char;
    add_character(c);
  }

  mark_for_redraw();
}

// adds a char to the input buffer
void Keyboard::add_character(char c) {
  if (buffer_length < max_length) {
    input_buffer[buffer_length] = c;
    buffer_length++;
    input_buffer[buffer_length] = '\0';
    mark_for_redraw();
  }
}

// main touch event handler
void Keyboard::handle_touch(uint16_t tx, uint16_t ty, lgfx::LGFX_Device* lcd) {
  // ignore touches outside keyboard area
  if (ty < KEYBOARD_Y_START) return;

  // debouncing: check if enough time passed since last touch
  unsigned long current_time = millis();
  if ((current_time - last_touch_time) < DEBOUNCE_DELAY) {
    return;  // ignore touch, too soon after last one
  }

  // get key index from touch
  int8_t key_index = find_touched_key(tx, ty);

  if (key_index >= 0) {
    // update debounce timer
    last_touch_time = current_time;

    // save press state for highlight
    pressed_key_index = key_index;
    press_timestamp = millis();

    // handle the key logic
    handle_key_press(key_index);

    // redraw immediately for press feedback
    draw(lcd);
  }
}