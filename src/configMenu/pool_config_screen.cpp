// pool_config_screen.cpp

#include "configMenu/pool_config_screen.h"

// constructor
PoolConfigScreen::PoolConfigScreen() {
  // initialize state variables
  mark_for_redraw();
  popup_needs_redraw = false;
  selected_item_index = -1;
  current_state = pool_screen_state_t::STATE_LIST;

  // initialize all 4 pool slots to empty state
  for (int i = 0; i < LIST_SLOTS; i++) {
    pools[i].address[0] = '\0';
    pools[i].name[0] = '\0';
    pools[i].is_active = false;
    pools[i].is_configured = false;
  }
}

// main draw function called every loop to render current state
void PoolConfigScreen::draw(lgfx::LGFX_Device* lcd) {
  switch (current_state) {
    case pool_screen_state_t::STATE_LIST:
      // draw pool list view with header
      UIUtils::draw_header(lcd, "Pools");
      draw_list(lcd);
      break;

    case pool_screen_state_t::STATE_ENTERING_NAME:
      // show keyboard for pool name input
      kb.draw(lcd);
      break;

    case pool_screen_state_t::STATE_ENTERING_ADDRESS:
      // show keyboard for pool address input
      kb.draw(lcd);
      break;

    case pool_screen_state_t::STATE_POPUP_MENU:
      // only redraw popup once to prevent flickering
      if (popup_needs_redraw) {
        UIUtils::draw_header(lcd, "Pools");
        draw_list(lcd);
        draw_popup_menu(lcd);
        popup_needs_redraw = false;
      }
      break;
  }
}

// handle touch input and route to appropriate state handler
void PoolConfigScreen::handle_touch(uint16_t tx, uint16_t ty, lgfx::LGFX_Device* lcd) {
  // debounce touch input
  if (!UIUtils::touch_debounce()) {
    return;
  }

  switch (current_state) {
    case pool_screen_state_t::STATE_LIST:
      // check if any pool item was touched
      for (int i = 0; i < LIST_SLOTS; i++) {
        uint16_t item_y = LIST_START_Y + (ITEM_HEIGHT * i);
        if (UIUtils::is_point_in_rect(tx, ty, LIST_START_X, item_y, SCREEN_WIDTH, ITEM_HEIGHT)) {
          // pool item touched open popup menu
          selected_item_index = i;
          popup_needs_redraw = true;
          current_state = pool_screen_state_t::STATE_POPUP_MENU;
        }
      }
      // check if back button was touched
      if (UIUtils::is_point_in_rect(tx, ty, BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_HEIGHT)) {
        // exit pool config interface
      }
      break;

    case pool_screen_state_t::STATE_ENTERING_NAME:
      // pass touch to keyboard for name input
      kb.handle_touch(tx, ty, lcd);

      if (kb.is_cancelled()) {
        // user cancelled return to list
        mark_for_redraw();
        current_state = pool_screen_state_t::STATE_LIST;
        kb.clear();
      } else if (kb.is_complete()) {
        // name entered successfully save to temp and move to address entry
        const char* pool_name = kb.get_text();
        strncpy(temp_name, pool_name, MAX_POOL_NAME_LENGTH);
        temp_name[MAX_POOL_NAME_LENGTH] = '\0';

        current_state = pool_screen_state_t::STATE_ENTERING_ADDRESS;
        kb.clear();
        kb.set_label("Enter pool address");
        kb.set_max_length(MAX_POOL_ADDRESS_LENGTH);
        kb.reset_complete();
      }
      break;

    case pool_screen_state_t::STATE_ENTERING_ADDRESS:
      // pass touch to keyboard for address input
      kb.handle_touch(tx, ty, lcd);

      if (kb.is_cancelled()) {
        // user cancelled go back to name entry
        mark_for_redraw();
        current_state = pool_screen_state_t::STATE_ENTERING_NAME;
        kb.clear();
      } else if (kb.is_complete()) {
        // address entered successfully save pool data
        const char* pool_address = kb.get_text();
        strncpy(temp_address, pool_address, MAX_POOL_ADDRESS_LENGTH);
        temp_address[MAX_POOL_ADDRESS_LENGTH] = '\0';

        // copy temp data to actual pool slot
        strcpy(pools[selected_item_index].name, temp_name);
        strcpy(pools[selected_item_index].address, temp_address);
        pools[selected_item_index].is_configured = true;

        // check if any other pool is already configured
        bool has_other_pools = false;
        for (int j = 0; j < 4; j++) {
          if (j != selected_item_index && pools[j].is_configured) {
            has_other_pools = true;
            break;
          }
        }

        // if this is the first pool make it active
        if (!has_other_pools) {
          pools[selected_item_index].is_active = true;
        }

        // save to nvs and return to list
        save_to_nvs(selected_item_index);
        mark_for_redraw();
        current_state = pool_screen_state_t::STATE_LIST;
        kb.reset_complete();
      }
      break;

    case pool_screen_state_t::STATE_POPUP_MENU:
      if (pools[selected_item_index].is_configured) {
        // configured pool has 4 buttons: select edit delete back
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
                // select button: deactivate all pools then activate this one
                for (int j = 0; j < 4; j++) {
                  pools[j].is_active = false;
                }
                pools[selected_item_index].is_active = true;
                current_state = pool_screen_state_t::STATE_LIST;
                mark_for_redraw();
                break;

              case 1:
                // edit button: start keyboard flow to edit pool
                current_state = pool_screen_state_t::STATE_ENTERING_NAME;
                kb.clear();
                kb.set_label("Enter pool name");
                kb.set_max_length(MAX_POOL_NAME_LENGTH);
                kb.reset_complete();
                break;

              case 2:
                // delete button: clear pool data and return to list
                pools[selected_item_index].name[0] = '\0';
                pools[selected_item_index].address[0] = '\0';
                pools[selected_item_index].is_active = false;
                pools[selected_item_index].is_configured = false;
                current_state = pool_screen_state_t::STATE_LIST;
                save_to_nvs(selected_item_index);
                mark_for_redraw();
                break;

              case 3:
                // back button: close popup return to list
                current_state = pool_screen_state_t::STATE_LIST;
                mark_for_redraw();
                break;

              default:
                break;
            }
          }
        }

      } else {
        // empty pool has 2 buttons: add and back centered in positions 1 and 2
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
                // add button: start keyboard flow to create new pool
                current_state = pool_screen_state_t::STATE_ENTERING_NAME;
                kb.clear();
                kb.set_label("Enter pool name");
                kb.set_max_length(MAX_POOL_NAME_LENGTH);
                kb.reset_complete();
                break;

              case 2:
                // back button: close popup return to list
                current_state = pool_screen_state_t::STATE_LIST;
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

// draw list of all 4 pool slots
void PoolConfigScreen::draw_list(lgfx::LGFX_Device* lcd) {
  // clear screen if needed
  if (display_needs_redraw) {
    lcd->fillScreen(COLOR_BLACK);
    display_needs_redraw = false;
  }

  // draw each pool item
  for (int i = 0; i < LIST_SLOTS; i++) {
    uint16_t item_y = LIST_START_Y + (i * (ITEM_HEIGHT + ITEM_GAP));
    draw_list_item(i, LIST_START_X, item_y, lcd);
  }
}

// draw individual pool item at given position
void PoolConfigScreen::draw_list_item(uint8_t index, uint16_t x, uint16_t y, lgfx::LGFX_Device* lcd) {
  lcd->setTextColor(COLOR_WHITE);
  lcd->setTextSize(2);

  // draw item border rectangle
  lcd->drawRect(x, y, SCREEN_WIDTH - (2 * x), ITEM_HEIGHT, COLOR_WHITE);

  if (!pools[index].is_configured) {
    // empty slot show add prompt
    lcd->setCursor(x + 3, y + 12);
    lcd->print("  + Add pool");
  } else {
    // configured slot show pool name on first line
    lcd->setCursor(x + 3, y + 5);
    lcd->print(pools[index].name);

    // show truncated address on second line with smaller text
    lcd->setTextSize(1);
    lcd->setCursor(x + 3, y + 25);

    int addr_len = strlen(pools[index].address);

    // only use <first 10>...<last 10> format if address is long enough
    if (addr_len > 23) {
      // create truncated version: first 10 chars + ... + last 10 chars
      char truncated[24];
      strncpy(truncated, pools[index].address, 10);
      truncated[10] = '.';
      truncated[11] = '.';
      truncated[12] = '.';
      strncpy(truncated + 13, pools[index].address + addr_len - 10, 10);
      truncated[23] = '\0';
      lcd->print(truncated);
    } else {
      // address is short enough to display as-is
      lcd->print(pools[index].address);
    }
  }

  // show active indicator if this pool is selected
  if (pools[index].is_active) {
    lcd->setCursor(SCREEN_WIDTH - (8 * x), y + 12);
    lcd->setTextSize(2);
    lcd->setTextColor(COLOR_GREEN);
    lcd->print("x");
  }
}

// draw popup menu overlay with buttons for current pool
void PoolConfigScreen::draw_popup_menu(lgfx::LGFX_Device* lcd) {
  // clear and draw popup background
  lcd->fillRect(POPUP_MENU_X, POPUP_MENU_Y, POPUP_MENU_WIDTH, POPUP_MENU_HEIGHT, COLOR_BLACK);
  lcd->drawRect(POPUP_MENU_X, POPUP_MENU_Y, POPUP_MENU_WIDTH, POPUP_MENU_HEIGHT, COLOR_NICEBLUE);

  // setup text properties for buttons
  lcd->setTextColor(COLOR_WHITE);
  lcd->setColor(COLOR_WHITE);
  lcd->setTextSize(1);

  if (pools[selected_item_index].is_configured) {
    // configured pool shows 4 buttons in a row
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
    // empty pool shows 2 centered buttons
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

void PoolConfigScreen::mark_for_redraw() {
  display_needs_redraw = true;
}

// save pool data to nvs storage
void PoolConfigScreen::save_to_nvs(uint8_t index) {
  Preferences prefs;
  prefs.begin("esp32btcminer", false);  // open namespace in read/write mode

  // create unique keys for this pool slot
  char name_key[16];
  char addr_key[16];
  char active_key[16];
  char config_key[16];

  sprintf(name_key, "pool%d_name", index);
  sprintf(addr_key, "pool%d_addr", index);
  sprintf(active_key, "pool%d_act", index);
  sprintf(config_key, "pool%d_cfg", index);

  // save pool data
  prefs.putString(name_key, pools[index].name);
  prefs.putString(addr_key, pools[index].address);
  prefs.putBool(active_key, pools[index].is_active);
  prefs.putBool(config_key, pools[index].is_configured);

  prefs.end();  // close namespace
}

// load all pool data from nvs storage
void PoolConfigScreen::load_from_nvs() {
  Preferences prefs;
  prefs.begin("esp32btcminer", true);  // open namespace in read-only mode

  // load all 4 pool slots
  for (int i = 0; i < LIST_SLOTS; i++) {
    char name_key[16];
    char addr_key[16];
    char active_key[16];
    char config_key[16];

    sprintf(name_key, "pool%d_name", i);
    sprintf(addr_key, "pool%d_addr", i);
    sprintf(active_key, "pool%d_act", i);
    sprintf(config_key, "pool%d_cfg", i);

    // load data with empty string as default
    String name = prefs.getString(name_key, "");
    String addr = prefs.getString(addr_key, "");
    pools[i].is_active = prefs.getBool(active_key, false);
    pools[i].is_configured = prefs.getBool(config_key, false);

    // copy strings to pool arrays
    strncpy(pools[i].name, name.c_str(), MAX_POOL_NAME_LENGTH);
    pools[i].name[MAX_POOL_NAME_LENGTH] = '\0';

    strncpy(pools[i].address, addr.c_str(), MAX_POOL_ADDRESS_LENGTH);
    pools[i].address[MAX_POOL_ADDRESS_LENGTH] = '\0';
  }

  prefs.end();

  // set defaults for slots 0 and 1 only if they are still empty after loading
  if (!pools[0].is_configured) {
    strcpy(pools[0].name, "Solo ckpool");
    strcpy(pools[0].address, "solo.ckpool.org:3333");
    pools[0].is_configured = true;
    pools[0].is_active = true;  // first default is active
  }

  if (!pools[1].is_configured) {
    strcpy(pools[1].name, "Public Pool");
    strcpy(pools[1].address, "public-pool.io:21496");
    pools[1].is_configured = true;
    pools[1].is_active = false;  // second default is not active
  }
}