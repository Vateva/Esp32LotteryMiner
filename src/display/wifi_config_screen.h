// wifi_config_screen.h
// handles wifi network scanning, selection, and connection
#ifndef WIFI_CONFIG_SCREEN_H
#define WIFI_CONFIG_SCREEN_H

#include <Arduino.h>
#include <WiFi.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "config.h"
#include "keyboard.h"

// screen states for wifi configuration flow
enum screen_state_t {
    STATE_SCANNING,      // scanning for networks
    STATE_LIST,          // displaying network list with pagination
    STATE_PASSWORD,      // keyboard input for password
    STATE_CONNECTING,    // attempting connection
    STATE_SUCCESS,       // connection successful
    STATE_ERROR          // connection failed
};

// stores information about a single scanned network
struct network_info_t {
    char ssid[MAX_SSID_LENGTH + 1];  // network name
    int32_t rssi;                     // signal strength
    wifi_auth_mode_t encryption;      // security type
};

class WifiConfigScreen {
private:
    // network data
    network_info_t scanned_networks[20];  // up to 20 networks
    int8_t scanned_networks_amount;       // actual count found
    int8_t selected_network;              // index of selected network (-1 if none)
    
    // ui state
    screen_state_t current_state;         // current screen state
    int8_t current_page;                  // current page in network list
    Keyboard kb;                          // keyboard widget for password entry
    
    // drawing helpers - render specific ui elements
    void draw_signal_strength_bars(int32_t rssi, uint16_t x, uint16_t y, lgfx::LGFX_Device* lcd);
    void draw_message(uint16_t color, const char* message, uint16_t x, uint16_t y, lgfx::LGFX_Device* lcd);
    void draw_network_list(lgfx::LGFX_Device* lcd);
    void draw_network_list_item(const network_info_t& network, uint16_t x, uint16_t y, lgfx::LGFX_Device* lcd);
    void draw_page_buttons(lgfx::LGFX_Device* lcd);

public:
    WifiConfigScreen();
    
    // main interface methods
    void draw(lgfx::LGFX_Device* lcd);                                  // renders current state
    void clear();                                                       // resets screen state
    void handle_touch(uint16_t tx, uint16_t ty, lgfx::LGFX_Device* lcd);  // processes touch input
    void start_scan();                                                  // initiates network scan
    void connect();                                                     // connects to selected network
};

#endif