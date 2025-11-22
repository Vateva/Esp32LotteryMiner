// wallet_config_screen.cpp

#include "wallet_config_screen.h"

// constructor
WalletConfigScreen::WalletConfigScreen() {
  // initialize state variables
  mark_for_redraw();
  popup_needs_redraw = false;
  selected_item_index = -1;

  // initialize all 4 wallet slots to empty/default state
  for (int i = 0; i < LIST_SLOTS; i++) {
    wallets[i].address[0] = '\0';  // empty string
    wallets[i].name[0] = '\0';     // empty string
    wallets[i].is_active = false;
    wallets[i].is_configured = false;
  }
}

// main draw function called every loop to render current state
void WalletConfigScreen::draw(lgfx::LGFX_Device* lcd) {
  switch (current_state) {
    case wallet_screen_state_t::STATE_LIST:
      // draw wallet list view with header
      UIUtils::draw_header(lcd, "Wallets");
      draw_list(lcd);
      break;

    case wallet_screen_state_t::STATE_ENTERING_NAME:
      // show keyboard for wallet name input
      kb.draw(lcd);
      break;

    case wallet_screen_state_t::STATE_ENTERING_ADDRESS:
      // show keyboard for wallet address input
      kb.draw(lcd);
      break;

    case wallet_screen_state_t::STATE_POPUP_MENU:
      // only redraw popup once to prevent flickering
      if (popup_needs_redraw) {
        UIUtils::draw_header(lcd, "Wallets");
        draw_list(lcd);
        draw_popup_menu(lcd);
        popup_needs_redraw = false;
      }
      break;
  }
}

// handle touch input and route to appropriate state handler
void WalletConfigScreen::handle_touch(uint16_t tx, uint16_t ty, lgfx::LGFX_Device* lcd) {
  // debounce touch input
  if (!UIUtils::touch_debounce()) {
    return;
  }

  switch (current_state) {
    case wallet_screen_state_t::STATE_LIST:
      // check if any wallet item was touched
      for (int i = 0; i < LIST_SLOTS; i++) {
        uint16_t item_y = LIST_START_Y + (ITEM_HEIGHT * i);
        if (UIUtils::is_point_in_rect(tx, ty, LIST_START_X, item_y, SCREEN_WIDTH, ITEM_HEIGHT)) {
          // wallet item touched open popup menu
          selected_item_index = i;
          popup_needs_redraw = true;
          current_state = wallet_screen_state_t::STATE_POPUP_MENU;
        }
      }
      // check if back button was touched
      if (UIUtils::is_point_in_rect(tx, ty, BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_HEIGHT)) {
        // exit wallet config interface
      }
      break;

    case wallet_screen_state_t::STATE_ENTERING_NAME:
      // pass touch to keyboard for name input
      kb.handle_touch(tx, ty, lcd);

      if (kb.is_cancelled()) {
        // user cancelled return to list
        mark_for_redraw();
        current_state = wallet_screen_state_t::STATE_LIST;
        kb.clear();
      } else if (kb.is_complete()) {
        // name entered successfully save to temp and move to address entry
        const char* wallet_name = kb.get_text();
        strncpy(temp_name, wallet_name, MAX_WALLET_NAME_LENGTH);
        temp_name[MAX_WALLET_NAME_LENGTH] = '\0';

        current_state = wallet_screen_state_t::STATE_ENTERING_ADDRESS;
        kb.clear();
        kb.set_label("Enter wallet address");
        kb.set_max_length(MAX_WALLET_ADDRESS_LENGTH);
        kb.reset_complete();
      }
      break;

    case wallet_screen_state_t::STATE_ENTERING_ADDRESS:
      // pass touch to keyboard for address input
      kb.handle_touch(tx, ty, lcd);

      if (kb.is_cancelled()) {
        // user cancelled go back to name entry
        mark_for_redraw();
        current_state = wallet_screen_state_t::STATE_ENTERING_NAME;
        kb.clear();
      } else if (kb.is_complete()) {
        // address entered successfully save wallet data
        const char* wallet_address = kb.get_text();
        strncpy(temp_address, wallet_address, MAX_WALLET_ADDRESS_LENGTH);
        temp_address[MAX_WALLET_ADDRESS_LENGTH] = '\0';

        // copy temp data to actual wallet slot
        strcpy(wallets[selected_item_index].name, temp_name);
        strcpy(wallets[selected_item_index].address, temp_address);
        wallets[selected_item_index].is_configured = true;

        // check if any other wallet is already configured
        bool has_other_wallets = false;
        for (int j = 0; j < 4; j++) {
          if (j != selected_item_index && wallets[j].is_configured) {
            has_other_wallets = true;
            break;
          }
        }

        // if this is the first wallet make it active
        if (!has_other_wallets) {
          wallets[selected_item_index].is_active = true;
        }

        // save to nvs and return to list
        save_to_nvs(selected_item_index);
        mark_for_redraw();
        current_state = wallet_screen_state_t::STATE_LIST;
        kb.reset_complete();
      }
      break;

    case wallet_screen_state_t::STATE_POPUP_MENU:
      if (wallets[selected_item_index].is_configured) {
        // configured wallet has 4 buttons: select edit delete back
        for (int i = 0; i < 4; i++) {
          if (UIUtils::is_point_in_rect(
                  tx,
                  ty,
                  POPUP_MENU_BUTTON_X + (i * POPUP_MENU_BUTTON_WIDTH) + (i * POPUP_MENU_BUTTON_GAP),
                  POPUP_MENU_BUTTON_Y,
                  POPUP_MENU_BUTTON_WIDTH,
                  POPUP_MENU_BUTTON_HEIGHT)) {
            switch (i) {
              case 0:
                // select button: deactivate all wallets then activate this one
                for (int j = 0; j < 4; j++) {
                  wallets[j].is_active = false;
                }
                wallets[selected_item_index].is_active = true;
                current_state = wallet_screen_state_t::STATE_LIST;
                mark_for_redraw();
                break;

              case 1:
                // edit button: start keyboard flow to edit wallet
                current_state = wallet_screen_state_t::STATE_ENTERING_NAME;
                kb.clear();
                kb.set_label("Enter wallet name");
                kb.set_max_length(MAX_WALLET_NAME_LENGTH);
                kb.reset_complete();
                break;

              case 2:
                // delete button: clear wallet data and return to list
                wallets[selected_item_index].name[0] = '\0';
                wallets[selected_item_index].address[0] = '\0';
                wallets[selected_item_index].is_active = false;
                wallets[selected_item_index].is_configured = false;
                current_state = wallet_screen_state_t::STATE_LIST;
                save_to_nvs(selected_item_index);
                mark_for_redraw();
                break;

              case 3:
                // back button: close popup return to list
                current_state = wallet_screen_state_t::STATE_LIST;
                mark_for_redraw();
                break;

              default:
                break;
            }
          }
        }

      } else {
        // empty wallet has 2 buttons: add and back centered in positions 1 and 2
        for (int i = 1; i < 3; i++) {
          if (UIUtils::is_point_in_rect(
                  tx,
                  ty,
                  POPUP_MENU_BUTTON_X + (i * POPUP_MENU_BUTTON_WIDTH) + (i * POPUP_MENU_BUTTON_GAP),
                  POPUP_MENU_BUTTON_Y,
                  POPUP_MENU_BUTTON_WIDTH,
                  POPUP_MENU_BUTTON_HEIGHT)) {
            switch (i) {
              case 1:
                // add button: start keyboard flow to create new wallet
                current_state = wallet_screen_state_t::STATE_ENTERING_NAME;
                kb.clear();
                kb.set_label("Enter wallet name");
                kb.set_max_length(MAX_WALLET_NAME_LENGTH);
                kb.reset_complete();
                break;

              case 2:
                // back button: close popup return to list
                current_state = wallet_screen_state_t::STATE_LIST;
                mark_for_redraw();
                break;

              default:
                break;
            }
          }
        }
      }
      break;
  }
}

// draw list of all 4 wallet slots
void WalletConfigScreen::draw_list(lgfx::LGFX_Device* lcd) {
  // clear screen if needed
  if (display_needs_redraw) {
    lcd->fillScreen(COLOR_BLACK);
    display_needs_redraw = false;
  }

  // draw each wallet item
  for (int i = 0; i < LIST_SLOTS; i++) {
    uint16_t item_y = LIST_START_Y + (i * (ITEM_HEIGHT + ITEM_GAP));
    draw_list_item(i, LIST_START_X, item_y, lcd);
  }
}

// draw individual wallet item at given position
void WalletConfigScreen::draw_list_item(uint8_t index, uint16_t x, uint16_t y, lgfx::LGFX_Device* lcd) {
  lcd->setTextColor(COLOR_WHITE);
  lcd->setTextSize(2);

  // draw item border rectangle
  lcd->drawRect(x, y, SCREEN_WIDTH - (2 * LIST_START_X), ITEM_HEIGHT, COLOR_WHITE);

  if (!wallets[index].is_configured) {
    // empty slot show add prompt
    lcd->setCursor(x + 3, y + 12);
    lcd->print("  + Add wallet");
  } else {
    // configured slot show wallet name on first line
    lcd->setCursor(x + 3, y + 5);
    lcd->print(wallets[index].name);

    // show truncated address on second line with smaller text
    lcd->setTextSize(1);
    lcd->setCursor(x + 3, y + 25);

    // create truncated version: first 10 chars + ... + last 10 chars
    char truncated[24];
    strncpy(truncated, wallets[index].address, 10);
    truncated[10] = '.';
    truncated[11] = '.';
    truncated[12] = '.';

    int addr_len = strlen(wallets[index].address);
    strncpy(truncated + 13, wallets[index].address + addr_len - 10, 10);
    truncated[23] = '\0';

    lcd->print(truncated);
  }

  // show active indicator if this wallet is selected
  if (wallets[index].is_active) {
    lcd->setCursor(SCREEN_WIDTH - (8 * x), y + 12);
    lcd->setTextSize(2);
    lcd->setTextColor(COLOR_GREEN);
    lcd->print("x");
  }
}

// draw popup menu overlay with buttons for current wallet
void WalletConfigScreen::draw_popup_menu(lgfx::LGFX_Device* lcd) {
  // clear and draw popup background
  lcd->fillRect(POPUP_MENU_X, POPUP_MENU_Y, POPUP_MENU_WIDTH, POPUP_MENU_HEIGHT, COLOR_BLACK);
  lcd->drawRect(POPUP_MENU_X, POPUP_MENU_Y, POPUP_MENU_WIDTH, POPUP_MENU_HEIGHT, COLOR_NICEBLUE);

  // setup text properties for buttons
  lcd->setTextColor(COLOR_WHITE);
  lcd->setColor(COLOR_WHITE);
  lcd->setTextSize(1);

  if (wallets[selected_item_index].is_configured) {
    // configured wallet shows 4 buttons in a row
    const char* button_labels[] = {"Select", "Edit", "Delete", "Back"};

    for (int i = 0; i < 4; i++) {
      // draw button rectangle
      lcd->drawRect(POPUP_MENU_BUTTON_X + (i * POPUP_MENU_BUTTON_WIDTH) + (i * POPUP_MENU_BUTTON_GAP),
                    POPUP_MENU_BUTTON_Y,
                    POPUP_MENU_BUTTON_WIDTH,
                    POPUP_MENU_BUTTON_HEIGHT);

      // draw button label
      lcd->setCursor(POPUP_MENU_BUTTON_TEXT_X + (i * POPUP_MENU_BUTTON_WIDTH) + (i * POPUP_MENU_BUTTON_GAP),
                     POPUP_MENU_BUTTON_TEXT_Y);
      lcd->print(button_labels[i]);
    }
  } else {
    // empty wallet shows 2 centered buttons
    const char* button_labels[] = {"Add", "Back"};

    for (int i = 0; i < 2; i++) {
      // calculate position to center buttons in positions 1 and 2
      int button_pos = i + 1;

      // draw button rectangle
      lcd->drawRect(POPUP_MENU_BUTTON_X + (button_pos * POPUP_MENU_BUTTON_WIDTH) + (button_pos * POPUP_MENU_BUTTON_GAP),
                    POPUP_MENU_BUTTON_Y,
                    POPUP_MENU_BUTTON_WIDTH,
                    POPUP_MENU_BUTTON_HEIGHT);

      // draw button label
      lcd->setCursor(
          POPUP_MENU_BUTTON_TEXT_X + (button_pos * POPUP_MENU_BUTTON_WIDTH) + (button_pos * POPUP_MENU_BUTTON_GAP),
          POPUP_MENU_BUTTON_TEXT_Y);
      lcd->print(button_labels[i]);
    }
  }
}

void WalletConfigScreen::mark_for_redraw() {
  display_needs_redraw = true;
}

// save wallet data to nvs storage
void WalletConfigScreen::save_to_nvs(uint8_t index) {
  Preferences prefs;
  prefs.begin("esp32btcminer", false);  // open namespace in read/write mode

  // create unique keys for this wallet slot
  char name_key[16];
  char addr_key[16];
  char active_key[16];
  char config_key[16];

  sprintf(name_key, "wallet%d_name", index);
  sprintf(addr_key, "wallet%d_addr", index);
  sprintf(active_key, "wallet%d_act", index);
  sprintf(config_key, "wallet%d_cfg", index);

  // save wallet data
  prefs.putString(name_key, wallets[index].name);
  prefs.putString(addr_key, wallets[index].address);
  prefs.putBool(active_key, wallets[index].is_active);
  prefs.putBool(config_key, wallets[index].is_configured);

  prefs.end();  // close namespace
}

// load all wallet data from nvs storage
void WalletConfigScreen::load_from_nvs() {
  Preferences prefs;
  prefs.begin("esp32btcminer", true);  // open namespace in read-only mode

  // load all 4 wallet slots
  for (int i = 0; i < LIST_SLOTS; i++) {
    char name_key[16];
    char addr_key[16];
    char active_key[16];
    char config_key[16];

    sprintf(name_key, "wallet%d_name", i);
    sprintf(addr_key, "wallet%d_addr", i);
    sprintf(active_key, "wallet%d_act", i);
    sprintf(config_key, "wallet%d_cfg", i);

    // load data with empty string as default
    String name = prefs.getString(name_key, "");
    String addr = prefs.getString(addr_key, "");
    wallets[i].is_active = prefs.getBool(active_key, false);
    wallets[i].is_configured = prefs.getBool(config_key, false);

    // copy strings to wallet arrays
    strncpy(wallets[i].name, name.c_str(), MAX_WALLET_NAME_LENGTH);
    wallets[i].name[MAX_WALLET_NAME_LENGTH] = '\0';

    strncpy(wallets[i].address, addr.c_str(), MAX_WALLET_ADDRESS_LENGTH);
    wallets[i].address[MAX_WALLET_ADDRESS_LENGTH] = '\0';
  }

  prefs.end();
}