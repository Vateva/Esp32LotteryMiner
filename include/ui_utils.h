// ui_utils.h
// common ui helper functions used across multiple screens
#ifndef UI_UTILS_H
#define UI_UTILS_H

#include <Arduino.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#include "config.h"

namespace UIUtils {

// draw back button arrow in top left corner
void draw_back_button(lgfx::LGFX_Device* lcd);

// draw header with title text and back button
void draw_header(lgfx::LGFX_Device* lcd, const char* title);

// check if touch point falls within rectangular area
bool is_point_in_rect(uint16_t touch_x,
                      uint16_t touch_y,
                      uint16_t rect_x,
                      uint16_t rect_y,
                      uint16_t rect_width,
                      uint16_t rect_height);

}  // namespace UIUtils

#endif