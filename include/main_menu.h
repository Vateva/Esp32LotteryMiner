// wallet_config_screen.h
#ifndef WALLET_CONFIG_SCREEN_H
#define WALLET_CONFIG_SCREEN_H

#include <Arduino.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#include "config.h"

class MainMenu {
 private:
  // ui state
  int8_t selected_menu_item;  // -1 = none, 0-2 = menu items
  bool display_needs_redraw;

  // touch debouncing
  unsigned long last_touch_time;
  static const uint16_t DEBOUNCE_DELAY = 200;

  // drawing methods
  void draw_menu_list(lgfx::LGFX_Device* lcd);
  void draw_menu_item(uint8_t index, uint16_t x, uint16_t y, lgfx::LGFX_Device* lcd);

 public:
  MainMenu();
  void draw(lgfx::LGFX_Device* lcd);
  void handle_touch(uint16_t tx, uint16_t ty, lgfx::LGFX_Device* lcd);
  int8_t get_selected_item() const;
  void reset_selection();
};

#endif