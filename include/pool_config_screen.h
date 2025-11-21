// pool_config_screen.h
#ifndef POOL_CONFIG_SCREEN_H
#define POOL_CONFIG_SCREEN_H

#include <Arduino.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <Preferences.h>

#include "config.h"
#include "ui_utils.h"
#include "keyboard.h"

// screen states for pool configuration flow
enum pool_screen_state_t {
  STATE_LIST,              // displaying pool list
  STATE_ENTERING_NAME,     // keyboard input for pool name
  STATE_ENTERING_ADDRESS,  // keyboard input for pool address
  STATE_POPUP_MENU         // showing edit/delete/activate options
};

class PoolConfigScreen {
 private:
  // pool data structure
  struct pool_info_t {
    char address[MAX_POOL_ADDRESS_LENGTH + 1];
    char name[MAX_POOL_NAME_LENGTH + 1];
    bool is_active;      // currently selected pool
    bool is_configured;  // slot has valid data
  };

  // pool storage
  pool_info_t pools[4];

  // ui state
  pool_screen_state_t current_state;
  int8_t selected_item_index;  // -1 = none, 0-3 = selected slot
  bool display_needs_redraw;
  bool popup_needs_redraw;

  // temporary storage for pool being edited
  char temp_name[MAX_POOL_NAME_LENGTH + 1];
  char temp_address[MAX_POOL_ADDRESS_LENGTH + 1];

  // keyboard widget
  Keyboard kb;


  // drawing methods
  void draw_back_button(lgfx::LGFX_Device* lcd);
  void draw_header(lgfx::LGFX_Device* lcd);
  void draw_list(lgfx::LGFX_Device* lcd);
  void draw_list_item(uint8_t index, uint16_t x, uint16_t y, lgfx::LGFX_Device* lcd);
  void draw_popup_menu(lgfx::LGFX_Device* lcd);

  // touch detection helpers
  bool is_point_in_rect(uint16_t touch_x,
                        uint16_t touch_y,
                        uint16_t rect_x,
                        uint16_t rect_y,
                        uint16_t rect_width,
                        uint16_t rect_height);

  // nvs persistence
  void save_to_nvs(uint8_t index);
  void load_from_nvs();

 public:
  // constructor
  PoolConfigScreen();

  // main interface
  void draw(lgfx::LGFX_Device* lcd);
  void handle_touch(uint16_t tx, uint16_t ty, lgfx::LGFX_Device* lcd);
};

#endif