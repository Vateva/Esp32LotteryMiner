// wallet_config_screen.cpp
#include "wallet_config_screen.h"


// constructor
WalletConfigScreen::WalletConfigScreen() {
  // initialize state variables
  need_redraw = true;
  item_selected_index = -1;
  last_touch_time = 0;

  // initialize all 4 wallet slots to empty/default state
  for (int i = 0; i < TOTAL_SLOTS; i++) {
    wallets[i].address[0] = '\0';  // empty string
    wallets[i].name[0] = '\0';     // empty string
    wallets[i].is_active = false;
    wallets[i].is_configured = false;
  }

  // load saved wallets from NVS (if any exist)
  load_from_nvs();
}

// main draw function - called when screen needs to be rendered
void WalletConfigScreen::draw(lgfx::LGFX_Device* lcd) {

}

// handle touch input
void WalletConfigScreen::handle_touch(uint16_t tx, uint16_t ty, lgfx::LGFX_Device* lcd) {

}

// private drawing methods
void WalletConfigScreen::draw_header(lgfx::LGFX_Device* lcd) {

}

void WalletConfigScreen::draw_back_button(lgfx::LGFX_Device* lcd) {

}

void WalletConfigScreen::draw_list(lgfx::LGFX_Device* lcd) {
 
}

void WalletConfigScreen::draw_list_item(uint8_t index, uint16_t y, lgfx::LGFX_Device* lcd) {

}

void WalletConfigScreen::draw_popup_menu(lgfx::LGFX_Device* lcd) {

}

void WalletConfigScreen::draw_popup_menu_buttons(lgfx::LGFX_Device* lcd) {

}

// Utility methods
bool WalletConfigScreen::is_point_in_rect(uint16_t touch_x,
                                          uint16_t touch_y,
                                          uint16_t rect_x,
                                          uint16_t rect_y,
                                          uint16_t rect_width,
                                          uint16_t rect_height) {
return (touch_x >= rect_x && touch_x < rect_x + rect_width && touch_y >= rect_y && touch_y < rect_y + rect_height);
}

// NVS storage methods
void WalletConfigScreen::save_to_nvs(uint8_t index) {

}

void WalletConfigScreen::load_from_nvs() {

}