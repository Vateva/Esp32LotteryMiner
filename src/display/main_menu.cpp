// main_menu.cpp

#include "main_menu.h"

// constructor
MainMenu::MainMenu() {
  selected_menu_item = -1;  // -1 = none, 0-2 = menu items
  mark_for_redraw();
}

// main draw function called every loop to render current state
void MainMenu::draw(lgfx::LGFX_Device* lcd) {
  UIUtils::draw_header(lcd, "Main menu");
  draw_menu_list(lcd);
}

// handle touch input and detect which menu item was touched
void MainMenu::handle_touch(uint16_t tx, uint16_t ty, lgfx::LGFX_Device* lcd) {
  // debounce touch input to prevent multiple rapid triggers
  if (!UIUtils::touch_debounce()) {
    return;
  }

  // check if any menu item was touched
  for (int i = 0; i < LIST_SLOTS; i++) {
    uint16_t item_y = LIST_START_Y + (ITEM_HEIGHT * i);
    if (UIUtils::is_point_in_rect(tx, ty, LIST_START_X, item_y, SCREEN_WIDTH, ITEM_HEIGHT)) {
      // menu item touched set selected_menu_index
      selected_menu_item = i;
    }
  }
  // check if back button was touched
  if (UIUtils::is_point_in_rect(tx, ty, BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_HEIGHT)) {
    // exit main menu interface
  }
}

// return currently selected menu item index
int8_t MainMenu::get_selected_item() const {
  return selected_menu_item;
}

// clear selection after navigation
void MainMenu::reset_selection() {
  selected_menu_item = -1;
}

// draw list of 3 menu items
void MainMenu::draw_menu_list(lgfx::LGFX_Device* lcd) {
  // draw list of all 4 menu options

  // clear screen if needed
  if (display_needs_redraw) {
    lcd->fillScreen(COLOR_BLACK);
    display_needs_redraw = false;
  }

  // draw each pool item
  for (int i = 0; i < LIST_SLOTS; i++) {
    uint16_t item_y = LIST_START_Y + (i * (ITEM_HEIGHT + ITEM_GAP));
    draw_menu_item(i, LIST_START_X, item_y, lcd);
  }
}

// draw individual menu item at given position
void MainMenu::draw_menu_item(uint8_t index, uint16_t x, uint16_t y, lgfx::LGFX_Device* lcd) {
  lcd->setTextColor(COLOR_WHITE);
  lcd->setTextSize(2);

  // draw item border rectangle
  lcd->drawRect(x, y, SCREEN_WIDTH - (2 * LIST_START_X), ITEM_HEIGHT, COLOR_WHITE);

  // menu options
  const char* menu_options[] = {"Wallets", "Pools", "WiFi", "Themes"};

  lcd->setCursor(x + 3, y + 15);  // x & y internal button margin
  lcd->print(menu_options[index]);
}
void MainMenu::mark_for_redraw() {
  display_needs_redraw = true;
}