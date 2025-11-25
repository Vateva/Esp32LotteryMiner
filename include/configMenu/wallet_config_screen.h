// wallet_config_screen.h
#ifndef WALLET_CONFIG_SCREEN_H
#define WALLET_CONFIG_SCREEN_H

#include <Arduino.h>
#define LGFX_USE_V1
#include <Preferences.h>

#include <LovyanGFX.hpp>

#include "config.h"
#include "keyboard.h"
#include "ui_utils.h"

// screen states for wallet configuration flow
enum class wallet_screen_state_t {
  STATE_LIST,              // displaying wallet list
  STATE_ENTERING_NAME,     // keyboard input for wallet name
  STATE_ENTERING_ADDRESS,  // keyboard input for wallet address
  STATE_POPUP_MENU         // showing edit/delete/activate options
};

class WalletConfigScreen {
 private:
  // wallet data structure
  struct wallet_info_t {
    char address[MAX_WALLET_ADDRESS_LENGTH + 1];
    char name[MAX_WALLET_NAME_LENGTH + 1];
    bool is_active;      // currently selected wallet
    bool is_configured;  // slot has valid data
  };

  // wallet storage
  wallet_info_t wallets[4];

  // ui state
  wallet_screen_state_t current_state;
  int8_t selected_item_index;  // -1 = none, 0-3 = selected slot
  bool display_needs_redraw;
  bool popup_needs_redraw;
  

  // temporary storage for wallet being edited
  char temp_name[MAX_WALLET_NAME_LENGTH + 1];
  char temp_address[MAX_WALLET_ADDRESS_LENGTH + 1];

  // keyboard widget
  Keyboard kb;

  // drawing methods
  void draw_list(lgfx::LGFX_Device* lcd);
  void draw_list_item(uint8_t index, uint16_t x, uint16_t y, lgfx::LGFX_Device* lcd);
  void draw_popup_menu(lgfx::LGFX_Device* lcd);
  

  // nvs persistence
  void save_to_nvs(uint8_t index);
  

 public:
  // constructor
  WalletConfigScreen();

  // main interface
  void draw(lgfx::LGFX_Device* lcd);
  void mark_for_redraw();

  void handle_touch(uint16_t tx, uint16_t ty, lgfx::LGFX_Device* lcd);

  void load_from_nvs();
};

#endif