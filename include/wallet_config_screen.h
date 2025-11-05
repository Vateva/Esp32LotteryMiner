// config_list_screen.h
#ifndef WALLET_CONFIG_SCREEN_H
#define WALLET_CONFIG_SCREEN_H

#include <Arduino.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#include "config.h"
#include "keyboard.h"

enum wallet_screen_state_t {
  STATE_LIST,           // showing the 4 wallet list
  STATE_ENTERING_NAME,  // keyboard for name
  STATE_ENTERING_ADDRESS, // keyboard for address
  STATE_POPUP_MENU      // showing edit/delete/activate menu
};

class WalletConfigScreen {
 private:
  struct wallet_info_t {
    char address[MAX_WALLET_ADDRESS_LENGTH + 1];
    char name[MAX_WALLET_NAME_LENGTH + 1];
    bool is_active;
    bool is_configured;
  };
  wallet_info_t wallets[4];  // the actual 4 wallet slots
  static const uint8_t TOTAL_SLOTS = 4;
  bool need_redraw;
  uint8_t item_selected_index;//flag to check if item selected and if so open popup menu
  Keyboard kb;

  // debouncing
  unsigned long last_touch_time;
  static const uint16_t DEBOUNCE_DELAY = 200;  // milliseconds

  void draw_back_button(lgfx::LGFX_Device* lcd);
  void draw_header(lgfx::LGFX_Device* lcd);
  void draw_list_item(uint8_t index, uint16_t y, lgfx::LGFX_Device* lcd);
  void draw_list(lgfx::LGFX_Device* lcd);
  void draw_popup_menu(lgfx::LGFX_Device* lcd);
  void draw_popup_menu_buttons(lgfx::LGFX_Device* lcd);
  bool is_point_in_rect(uint16_t touch_x,
                        uint16_t touch_y,
                        uint16_t rect_x,
                        uint16_t rect_y,
                        uint16_t rect_width,
                        uint16_t rect_height);
  void save_to_nvs(uint8_t index);  // save specific wallet to nvs
void load_from_nvs();  // load all wallets from nvs
  


 public:
  // constructor
  WalletConfigScreen();

  // methods base class provides
  void draw(lgfx::LGFX_Device* lcd);
  void handle_touch(uint16_t tx, uint16_t ty, lgfx::LGFX_Device* lcd);
};

#endif