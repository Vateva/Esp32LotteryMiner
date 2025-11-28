// theme_config_screen.cpp
// theme selection screen for homepage appearance

#include "configMenu/theme_config_screen.h"

// constructor
ThemeConfigScreen::ThemeConfigScreen() {
    mark_for_redraw();
    
    // initialize predefined themes
    strcpy(themes[0].name, "Homepage 1");
    themes[0].is_active = true;  // first theme active by default
    
    strcpy(themes[1].name, "Homepage 2");
    themes[1].is_active = false;
    
    strcpy(themes[2].name, "Homepage 3");
    themes[2].is_active = false;
    
    strcpy(themes[3].name, "Homepage 4");
    themes[3].is_active = false;
}

// main draw function
void ThemeConfigScreen::draw(lgfx::LGFX_Device* lcd) {
    UIUtils::draw_header(lcd, "Themes");
    draw_list(lcd);
}

// handle touch input
void ThemeConfigScreen::handle_touch(uint16_t tx, uint16_t ty, lgfx::LGFX_Device* lcd) {
    // debounce touch input
    if (!UIUtils::touch_debounce()) {
        return;
    }
    
    // check if any theme item was touched
    for (int i = 0; i < THEME_COUNT; i++) {
        uint16_t item_y = LIST_START_Y + (i * (ITEM_HEIGHT + ITEM_GAP));
        
        if (UIUtils::is_point_in_rect(tx, ty, LIST_START_X, item_y, SCREEN_WIDTH - (2 * LIST_START_X), ITEM_HEIGHT)) {
            // deactivate all themes
            for (int j = 0; j < THEME_COUNT; j++) {
                themes[j].is_active = false;
            }
            
            // activate touched theme
            themes[i].is_active = true;
            
            // save selection to nvs
            save_to_nvs();
            
            // redraw to show new selection
            mark_for_redraw();
            
            return;
        }
    }
    
    // check if back button was touched
    if (UIUtils::is_point_in_rect(tx, ty, BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_HEIGHT)) {
        // back button handled by main navigation
    }
}

// draw list of all theme slots
void ThemeConfigScreen::draw_list(lgfx::LGFX_Device* lcd) {
    // clear screen if needed
    if (display_needs_redraw) {
        lcd->fillScreen(COLOR_BLACK);
        display_needs_redraw = false;
    }
    
    // draw each theme item
    for (int i = 0; i < THEME_COUNT; i++) {
        uint16_t item_y = LIST_START_Y + (i * (ITEM_HEIGHT + ITEM_GAP));
        draw_list_item(i, LIST_START_X, item_y, lcd);
    }
}

// draw individual theme item
void ThemeConfigScreen::draw_list_item(uint8_t index, uint16_t x, uint16_t y, lgfx::LGFX_Device* lcd) {
    lcd->setTextColor(COLOR_WHITE);
    lcd->setTextSize(2);
    
    // draw item border rectangle
    lcd->drawRect(x, y, SCREEN_WIDTH - (2 * x), ITEM_HEIGHT, COLOR_WHITE);
    
    // draw theme name
    lcd->setCursor(x + 3, y + 12);
    lcd->print(themes[index].name);
    
    // show active indicator if this theme is selected
    if (themes[index].is_active) {
        lcd->setCursor(SCREEN_WIDTH - (8 * x), y + 12);
        lcd->setTextColor(COLOR_GREEN);
        lcd->print("x");
    }
}

// force screen redraw
void ThemeConfigScreen::mark_for_redraw() {
    display_needs_redraw = true;
}

// save active theme to nvs
void ThemeConfigScreen::save_to_nvs() {
    Preferences prefs;
    prefs.begin("esp32btcminer", false);  // read/write mode
    
    // find active theme index and save it
    for (int i = 0; i < THEME_COUNT; i++) {
        if (themes[i].is_active) {
            prefs.putUChar("theme_active", i);
            break;
        }
    }
    
    prefs.end();
}

// load active theme from nvs
void ThemeConfigScreen::load_from_nvs() {
    Preferences prefs;
    prefs.begin("esp32btcminer", true);  // read-only mode
    
    // load active theme index (default to 0 if not set)
    uint8_t active_index = prefs.getUChar("theme_active", 0);
    
    // validate index
    if (active_index >= THEME_COUNT) {
        active_index = 0;
    }
    
    // set active theme
    for (int i = 0; i < THEME_COUNT; i++) {
        themes[i].is_active = (i == active_index);
    }
    
    prefs.end();
}

// get active theme index for home screen to use
uint8_t ThemeConfigScreen::get_active_theme_index() {
    for (int i = 0; i < THEME_COUNT; i++) {
        if (themes[i].is_active) {
            return i;
        }
    }
    return 0;  // fallback to first theme
}