// wifi_config_screen.cpp
#include "wifi_config_screen.h"

// constructor - initialize screen state
WifiConfigScreen::WifiConfigScreen() {
        current_state = STATE_SCANNING;
        current_page = 0;
        selected_network = -1;
        scanned_networks_amount = 0;
}

// initiates async wifi network scan
void WifiConfigScreen::start_scan() {
        current_state = STATE_SCANNING;
        WiFi.scanNetworks(true, false);
}

// main draw method - renders current state
void WifiConfigScreen::draw(lgfx::LGFX_Device* lcd) {
        switch (current_state) {
                case STATE_SCANNING: {
                        int scan_status = WiFi.scanComplete();

                        if (scan_status == -2) {
                                // scan failed
                                Serial.println("wifi scan failed. restarting scan.");
                                start_scan();
                        } else if (scan_status == -1) {
                                // still scanning
                                draw_message(COLOR_YELLOW, "scanning networks...", true, 120, 180, lcd);
                        } else if (scan_status == 0) {
                                // no networks found
                                draw_message(COLOR_YELLOW, "no networks found.", false, 120, 180, lcd);
                                draw_manual_entry_button(lcd);
                        } else {
                                // networks found - process results
                                int networks_found = min(scan_status, 50);

                                // temporary array for all found networks
                                network_info_t temp_networks[50];

                                // copy data from wifi scan results
                                for (int i = 0; i < networks_found; i++) {
                                        strncpy(temp_networks[i].ssid, WiFi.SSID(i).c_str(), MAX_SSID_LENGTH);
                                        temp_networks[i].ssid[MAX_SSID_LENGTH] = '\0';
                                        temp_networks[i].rssi = WiFi.RSSI(i);
                                        temp_networks[i].encryption = WiFi.encryptionType(i);
                                }

                                // bubble sort networks by signal strength (strongest first)
                                for (int i = 0; i < networks_found - 1; i++) {
                                        for (int j = 0; j < networks_found - i - 1; j++) {
                                                if (temp_networks[j].rssi < temp_networks[j + 1].rssi) {
                                                        network_info_t temp_swap = temp_networks[j + 1];
                                                        temp_networks[j + 1] = temp_networks[j];
                                                        temp_networks[j] = temp_swap;
                                                }
                                        }
                                }

                                // copy top 20 to scanned_networks array
                                int networks_to_copy = min(networks_found, 20);

                                for (int i = 0; i < networks_to_copy; i++) {
                                        strncpy(scanned_networks[i].ssid, temp_networks[i].ssid, MAX_SSID_LENGTH);
                                        scanned_networks[i].ssid[MAX_SSID_LENGTH] = '\0';
                                        scanned_networks[i].rssi = temp_networks[i].rssi;
                                        scanned_networks[i].encryption = temp_networks[i].encryption;
                                }

                                // cleanup and transition to list view
                                WiFi.scanDelete();
                                scanned_networks_amount = networks_to_copy;
                                current_state = STATE_LIST;
                        }
                        break;
                }

                case STATE_LIST:
                        draw_network_list(lcd);
                        break;

                case STATE_PASSWORD:
                        kb.draw(lcd);
                        break;

                case STATE_SSID_MANUAL_ENTRY:

                        break;

                case STATE_PASSWORD_MANUAL_ENTRY:

                        break;

                case STATE_CONNECTING:

                        break;

                case STATE_SUCCESS:

                        break;

                case STATE_ERROR:

                        break;
        }
}

// placeholder methods - to be implemented
void WifiConfigScreen::clear() {}

void WifiConfigScreen::handle_touch(uint16_t tx, uint16_t ty, lgfx::LGFX_Device* lcd) {}

void WifiConfigScreen::connect() {}

// drawing helper placeholders
void WifiConfigScreen::draw_signal_strength_bars(int32_t rssi, uint16_t x, uint16_t y, lgfx::LGFX_Device* lcd) {}

void WifiConfigScreen::draw_message(uint16_t color,
                                    const char* message,
                                    bool animated,
                                    uint16_t x,
                                    uint16_t y,
                                    lgfx::LGFX_Device* lcd) {}

void WifiConfigScreen::draw_network_list(lgfx::LGFX_Device* lcd) {}

void WifiConfigScreen::draw_network_list_item(const network_info_t& network,
                                              uint16_t x,
                                              uint16_t y,
                                              lgfx::LGFX_Device* lcd) {}

void WifiConfigScreen::draw_page_buttons(lgfx::LGFX_Device* lcd) {}

void WifiConfigScreen::draw_manual_entry_button(lgfx::LGFX_Device* lcd) {}