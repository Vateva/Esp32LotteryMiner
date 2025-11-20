// ui_utils.cpp

#include "ui_utils.h"

namespace UIUtils {

// draw back button in top left corner
void draw_back_button(lgfx::LGFX_Device* lcd) {
  lcd->setTextColor(COLOR_WHITE);
  lcd->setColor(COLOR_WHITE);
  lcd->setTextSize(2);

  // draw button rectangle
  lcd->drawRect(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_HEIGHT);

  // draw arrow text
  lcd->setCursor(BACK_BUTTON_TEXT_X, BACK_BUTTON_TEXT_Y);
  lcd->print("<-");
}

// draw header with back button and title text
void draw_header(lgfx::LGFX_Device* lcd, const char* title) {
  // draw back button first
  draw_back_button(lcd);

  // draw title text
  lcd->setTextColor(COLOR_WHITE);
  lcd->setTextSize(2);
  lcd->setCursor(HEADER_TEXT_X, HEADER_TEXT_Y);
  lcd->print(title);
}

// check if touch coordinates fall within rectangle bounds
bool is_point_in_rect(uint16_t touch_x,
                      uint16_t touch_y,
                      uint16_t rect_x,
                      uint16_t rect_y,
                      uint16_t rect_width,
                      uint16_t rect_height) {
  return (touch_x >= rect_x && touch_x < rect_x + rect_width && touch_y >= rect_y &&
          touch_y < rect_y + rect_height);
}

}  // namespace UIUtils