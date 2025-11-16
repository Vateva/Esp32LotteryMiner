// wallet_config_screen.cpp
#include "wallet_config_screen.h"

// constructor
WalletConfigScreen::WalletConfigScreen() {
  // initialize state variables
  display_needs_redraw = true;
  selected_item_index = -1;
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
  switch (current_state) {
    case STATE_LIST:
      draw_header(lcd);
      draw_list(lcd);
      break;

    case STATE_ENTERING_NAME:
      kb.draw(lcd);
      break;

    case STATE_ENTERING_ADDRESS:
      kb.draw(lcd);
      break;

    case STATE_POPUP_MENU:
      draw_header(lcd);
      draw_list(lcd);
      draw_popup_menu(lcd);
      break;
  }
}

// handle touch input
void WalletConfigScreen::handle_touch(uint16_t tx, uint16_t ty, lgfx::LGFX_Device* lcd) {
  // debounce
  unsigned long current_time = millis();
  if ((current_time - last_touch_time) < DEBOUNCE_DELAY) {
    return;
  }
  // update debounce timer
  last_touch_time = current_time;
  switch (current_state) {
    case STATE_LIST:
      // handle touches in list view
      for (int i = 0; i < TOTAL_SLOTS; i++) {
        uint16_t item_y = LIST_START_Y + (ITEM_HEIGHT * i);
        if (is_point_in_rect(tx, ty, LIST_START_X, item_y, SCREEN_WIDTH, ITEM_HEIGHT)) {
          current_state = STATE_POPUP_MENU;
        }
      }
      break;

    case STATE_ENTERING_NAME:
      // handle keyboard input for name
      break;

    case STATE_ENTERING_ADDRESS:
      // handle keyboard input for address
      break;

    case STATE_POPUP_MENU:
      // handle popup menu button touches
      break;
  }
}

// private drawing methods
void WalletConfigScreen::draw_header(lgfx::LGFX_Device* lcd) {
  // back button
  draw_back_button(lcd);

  // draw header text
  lcd->setTextColor(COLOR_WHITE);
  lcd->setTextSize(2);
  lcd->setCursor(HEADER_TEXT_X, HEADER_TEXT_Y);
  lcd->print("Wallets");
}

void WalletConfigScreen::draw_back_button(lgfx::LGFX_Device* lcd) {
  lcd->setTextColor(COLOR_WHITE);
  lcd->setColor(COLOR_WHITE);
  lcd->setTextSize(2);

  lcd->drawRect(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_HEIGHT);
  lcd->setCursor(BACK_BUTTON_TEXT_X, BACK_BUTTON_TEXT_Y);
  lcd->print("<-");
}

void WalletConfigScreen::draw_list(lgfx::LGFX_Device* lcd) {
  if (display_needs_redraw) {
    lcd->fillScreen(COLOR_BLACK);
    display_needs_redraw = false;
  }

  for (int i = 0; i < TOTAL_SLOTS; i++) {
    uint16_t item_y = LIST_START_Y + (i * (ITEM_HEIGHT + ITEM_GAP));
    draw_list_item(i, LIST_START_X, item_y, lcd);
  }
}

void WalletConfigScreen::draw_list_item(uint8_t index, uint16_t x, uint16_t y, lgfx::LGFX_Device* lcd) {
  // draw rectangle
  lcd->drawRect(LIST_START_X, y, SCREEN_WIDTH - (2 * LIST_START_X), ITEM_HEIGHT);

  if (!wallets[index].is_configured) {
    // empty slot "+   Add wallet"
    lcd->setCursor(LIST_START_X + 3, y + 12);
    lcd->print("+   Add wallet");
  } else {
    // configured slot - show name on first line
    lcd->setCursor(LIST_START_X + 3, y + 5);
    lcd->setTextSize(2);
    lcd->print(wallets[index].name);  // or truncate if too long

    // show truncated address on second line, smaller text
    lcd->setTextSize(1);
    lcd->setCursor(LIST_START_X + 3, y + 25);

    char truncated[16];

    // copy first 4 chars
    strncpy(truncated, wallets[index].address, 6);

    // add dots at positions 4, 5, 6
    truncated[6] = '.';
    truncated[7] = '.';
    truncated[8] = '.';

    // find actual address length
    int addr_len = strlen(wallets[index].address);

    // copy last 4 chars from the end of the actual address
    strncpy(truncated + 9, wallets[index].address + addr_len - 6, 6);

    // null terminate
    truncated[15] = '\0';
  }

  // if active wallet, maybe draw a star or highlight
  if (wallets[index].is_active) {
    lcd->setCursor(SCREEN_WIDTH - (8 * LIST_START_X), y + 12);
    lcd->print("<-");
  }
}

void WalletConfigScreen::draw_popup_menu(lgfx::LGFX_Device* lcd) {
  // clear popup menu area
  lcd->fillRect(POPUP_MENU_X, POPUP_MENU_Y, POPUP_MENU_WIDTH, POPUP_MENU_HEIGHT, COLOR_BLACK);
  // draw popup menu frame
  lcd->drawRect(POPUP_MENU_X, POPUP_MENU_Y, POPUP_MENU_WIDTH, POPUP_MENU_HEIGHT);

  // draw buttons
  lcd->setTextColor(COLOR_WHITE);
  lcd->setColor(COLOR_WHITE);
  lcd->setTextSize(1);
  
  if (wallets[selected_item_index].is_configured) {
    // draw select/edit/delete/back buttons
    const char* button_labels[] = {"Select", "Edit", "Delete", "Back"};
    
    for (int i = 0; i < 4; i++) {
      lcd->drawRect(POPUP_MENU_BUTTON_X + (i * POPUP_MENU_BUTTON_WIDTH) + (i * POPUP_MENU_BUTTON_GAP),
                    POPUP_MENU_BUTTON_Y,
                    POPUP_MENU_BUTTON_WIDTH,
                    POPUP_MENU_BUTTON_HEIGHT);
      
      lcd->setCursor(POPUP_MENU_BUTTON_TEXT_X + (i * POPUP_MENU_BUTTON_WIDTH) + (i * POPUP_MENU_BUTTON_GAP),
                     POPUP_MENU_BUTTON_TEXT_Y);
      lcd->print(button_labels[i]);
    }
  } else {
    // draw add/back buttons centered
    const char* button_labels[] = {"Add", "Back"};
    
    for (int i = 1; i < 3; i++) {//buttons centered in positions 1-2

      
      lcd->drawRect(POPUP_MENU_BUTTON_X + (i * POPUP_MENU_BUTTON_WIDTH) + (i * POPUP_MENU_BUTTON_GAP),
                    POPUP_MENU_BUTTON_Y,
                    POPUP_MENU_BUTTON_WIDTH,
                    POPUP_MENU_BUTTON_HEIGHT);
      
      lcd->setCursor(POPUP_MENU_BUTTON_TEXT_X + (i * POPUP_MENU_BUTTON_WIDTH) + (i * POPUP_MENU_BUTTON_GAP),
                     POPUP_MENU_BUTTON_TEXT_Y);
      lcd->print(button_labels[i]);
    }
  }
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
void WalletConfigScreen::save_to_nvs(uint8_t index) {}

void WalletConfigScreen::load_from_nvs() {}