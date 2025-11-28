// theme_config_screen.h
// theme selection screen for homepage appearance

#ifndef THEME_CONFIG_SCREEN_H
#define THEME_CONFIG_SCREEN_H

#include <Arduino.h>
#include <LovyanGFX.hpp>
#include <Preferences.h>
#include "configMenu/ui_utils.h"

// number of available themes
#define THEME_COUNT 4

// theme data structure
struct theme_t {
    char name[16];          // display name
    bool is_active;         // currently selected
};

class ThemeConfigScreen {
public:
    ThemeConfigScreen();
    
    // main interface methods called from main loop
    void draw(lgfx::LGFX_Device* lcd);
    void handle_touch(uint16_t tx, uint16_t ty, lgfx::LGFX_Device* lcd);
    
    // persistence
    void save_to_nvs();
    void load_from_nvs();
    
    // get active theme index (0-3) for home screen to use
    uint8_t get_active_theme_index();
    
    // force screen redraw
    void mark_for_redraw();

private:
    // theme slots
    theme_t themes[THEME_COUNT];
    
    // display state
    bool display_needs_redraw;
    
    // drawing helpers
    void draw_list(lgfx::LGFX_Device* lcd);
    void draw_list_item(uint8_t index, uint16_t x, uint16_t y, lgfx::LGFX_Device* lcd);
};

#endif