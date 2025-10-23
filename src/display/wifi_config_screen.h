// wifi_config_screen.h
#ifndef WIFI_CONFIG_SCREEN_H
#define WIFI_CONFIG_SCREEN_H

#include <Arduino.h>
#include <WiFi.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "config.h"
#include "keyboard.h"

// screen states
enum screen_state_t {
    STATE_SCANNING,
    STATE_LIST,
    STATE_PASSWORD,
    STATE_CONNECTING,
    STATE_SUCCESS,
    STATE_ERROR
};

// network information structure
struct network_info_t {
    char ssid[MAX_SSID_LENGTH + 1];
    int32_t rssi;
    wifi_auth_mode_t encryption;
};

class WifiConfigScreen {
private:
    // state and data
    network_info_t scanned_networks[20];
    screen_state_t current_state;
    Keyboard kb;
    int8_t current_page;
    int8_t selected_network;
    int8_t scanned_networks_amount;
    
    // private drawing helpers
    void draw_signal_strength_bars(int32_t rssi, uint16_t x, uint16_t y, lgfx::LGFX_Device* lcd);
    void draw_message(uint16_t color, const char* message, uint16_t x, uint16_t y, lgfx::LGFX_Device* lcd);
    void draw_network_list(lgfx::LGFX_Device* lcd);
    void draw_network_list_item(const network_info_t& network, uint16_t x, uint16_t y, lgfx::LGFX_Device* lcd);
    void draw_page_buttons(lgfx::LGFX_Device* lcd);

public:
    WifiConfigScreen();
    
    void draw(lgfx::LGFX_Device* lcd);
    void clear();
    void handle_touch(uint16_t tx, uint16_t ty, lgfx::LGFX_Device* lcd);
    void start_scan();
    void connect();
};

#endif