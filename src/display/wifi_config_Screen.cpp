// wifi_config_screen.cpp
#include "wifi_config_screen.h"

// header area
const uint16_t HEADER_HEIGHT = 30;

// network list items
const uint16_t LIST_START_X = 2;
const uint16_t LIST_START_Y = 35;
const uint16_t ITEM_HEIGHT = 30;
const uint16_t ITEMS_PER_PAGE = 5;  // how many items fit?

// buttons at bottom
const uint16_t BOTTOM_BUTTONS_Y = 209;
const uint16_t BOTTOM_BUTTONS_TEXT_Y = 219;
const uint16_t BUTTON_HEIGHT = 30;

const uint16_t BUTTON_GAP = 5;

const uint16_t PREV_BUTTON_X = BUTTON_GAP;  // 5
const uint16_t PREV_BUTTON_W = 73;
const uint16_t PREV_BUTTON_TEXT_X = PREV_BUTTON_X + 10;  // 15

const uint16_t NEXT_BUTTON_X = PREV_BUTTON_X + PREV_BUTTON_W + BUTTON_GAP;  // 83
const uint16_t NEXT_BUTTON_W = 73;
const uint16_t NEXT_BUTTON_TEXT_X = NEXT_BUTTON_X + 10;  // 93

const uint16_t RESCAN_BUTTON_X = NEXT_BUTTON_X + NEXT_BUTTON_W + BUTTON_GAP;  // 161
const uint16_t RESCAN_BUTTON_W = 73;
const uint16_t RESCAN_BUTTON_TEXT_X = RESCAN_BUTTON_X + 5;  // 166 (shorter text needs less offset)

const uint16_t MANUAL_BUTTON_X = RESCAN_BUTTON_X + RESCAN_BUTTON_W + BUTTON_GAP;  // 239
const uint16_t MANUAL_BUTTON_W = 76;  // slightly wider to fill remaining space
const uint16_t MANUAL_BUTTON_TEXT_X = MANUAL_BUTTON_X + 5;  // 244

// strengh barrs parameters
const uint16_t BARR_W = 2;
const uint16_t BARR_1_H = 3;
const uint16_t BARR_2_H = 6;
const uint16_t BARR_3_H = 9;
const uint16_t BARR_4_H = 12;
const uint16_t BARR_2_X_OFFSET = 3;
const uint16_t BARR_3_X_OFFSET = 6;
const uint16_t BARR_4_X_OFFSET = 9;

// constructor - initialize screen state
WifiConfigScreen::WifiConfigScreen() {
  current_state = STATE_SCANNING;
  current_page = 0;
  selected_network = -1;
  scanned_networks_amount = 0;
  list_needs_redraw = true;  // redraw flag
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
        draw_bottom_buttons(lcd);
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
        list_needs_redraw = true;
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
      kb.draw(lcd);
      break;

    case STATE_PASSWORD_MANUAL_ENTRY:
      kb.draw(lcd);
      break;

    case STATE_CONNECTING: {
      int status = WiFi.status();

      if (status == WL_CONNECTED) {
        current_state = STATE_SUCCESS;
        state_change_time = millis();
      } else if (status == WL_CONNECT_FAILED || status == WL_NO_SSID_AVAIL) {
        current_state = STATE_ERROR;
        state_change_time = millis();
      } else if (millis() - connection_start_time > WIFI_CONNECT_TIMEOUT_MS) {
        current_state = STATE_ERROR;
        state_change_time = millis();
      } else {
        // still connecting, draw animation
        draw_message(COLOR_YELLOW, "connecting...", true, 180, 120, lcd);
      }
      break;
    }

    case STATE_SUCCESS:
      draw_message(COLOR_GREEN, "Connected!", false, 120, 180, lcd);
      if (millis() - state_change_time > 1500) {
        list_needs_redraw = true;
        current_state = STATE_LIST;
      }
      break;

    case STATE_ERROR:
      /*we can maybe later implement a more complete system with
      different error codes to display different messages*/
      draw_message(COLOR_RED, "Error connecting!", false, 120, 180, lcd);
      if (millis() - state_change_time > 1500) {
        list_needs_redraw = true;
        current_state = STATE_LIST;
      }
      break;
  }
}

// placeholder method
void WifiConfigScreen::clear() {}

void WifiConfigScreen::handle_touch(uint16_t tx, uint16_t ty, lgfx::LGFX_Device* lcd) {
  switch (current_state) {
    case STATE_LIST: {
      // check network item touches
      for (int i = 0; i < ITEMS_PER_PAGE; i++) {
        uint16_t item_y = LIST_START_Y + (ITEM_HEIGHT * i);
        if (is_point_in_rect(tx, ty, LIST_START_X, item_y, SCREEN_WIDTH, ITEM_HEIGHT)) {
          int network_index = (current_page * ITEMS_PER_PAGE) + i;

          if (network_index < scanned_networks_amount) {
            selected_network = network_index;

            if (scanned_networks[network_index].encryption == WIFI_AUTH_OPEN) {
              connect(scanned_networks[selected_network].ssid, "");
              current_state = STATE_CONNECTING;
            } else {
              current_state = STATE_PASSWORD;
              kb.clear();
              kb.set_max_length(MAX_WIFI_PASSWORD_LENGTH);
            }
            return;
          }
        }
      }

      // check button touches
      if (is_point_in_rect(tx, ty, PREV_BUTTON_X, BOTTOM_BUTTONS_Y, PREV_BUTTON_W, BUTTON_HEIGHT)) {
        if (current_page > 0) {
          list_needs_redraw = true;
          current_page--;
        }
      } else if (is_point_in_rect(tx, ty, NEXT_BUTTON_X, BOTTOM_BUTTONS_Y, NEXT_BUTTON_W, BUTTON_HEIGHT)) {
        int max_pages = (scanned_networks_amount + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE - 1;
        if (current_page < max_pages) {
          list_needs_redraw = true;
          current_page++;
        }
      } else if (is_point_in_rect(tx, ty, RESCAN_BUTTON_X, BOTTOM_BUTTONS_Y, RESCAN_BUTTON_W, BUTTON_HEIGHT)) {
        current_state = STATE_SCANNING;

      }  else if (is_point_in_rect(tx, ty, MANUAL_BUTTON_X, BOTTOM_BUTTONS_Y, MANUAL_BUTTON_W, BUTTON_HEIGHT)) {
        current_state = STATE_SSID_MANUAL_ENTRY;
        kb.clear();
        kb.set_max_length(MAX_SSID_LENGTH);
      }
      break;
    }

    case STATE_PASSWORD: {  // handle touch in password entering state
      kb.handle_touch(tx, ty, lcd);

      if (kb.is_complete()) {
        const char* password = kb.get_text();
        strncpy(typed_ssid_password, password, MAX_WIFI_PASSWORD_LENGTH);
        typed_ssid_password[MAX_WIFI_PASSWORD_LENGTH] = '\0';

        connect(scanned_networks[selected_network].ssid, typed_ssid_password);  // initiates wifi connection
        current_state = STATE_CONNECTING;                                       // show connecting animation
        kb.reset_complete();
      }
      break;
    }

    case STATE_SSID_MANUAL_ENTRY:
      kb.handle_touch(tx, ty, lcd);

      if (kb.is_complete()) {
        const char* ssid = kb.get_text();
        strncpy(manual_ssid, ssid, MAX_SSID_LENGTH);
        manual_ssid[MAX_SSID_LENGTH] = '\0';

        current_state = STATE_PASSWORD_MANUAL_ENTRY;
        kb.clear();  // show connecting animation
        kb.set_max_length(MAX_WIFI_PASSWORD_LENGTH);
        kb.reset_complete();
      }
      break;

    case STATE_PASSWORD_MANUAL_ENTRY:
      kb.handle_touch(tx, ty, lcd);

      if (kb.is_complete()) {
        const char* password = kb.get_text();
        strncpy(typed_ssid_password, password, MAX_WIFI_PASSWORD_LENGTH);
        typed_ssid_password[MAX_WIFI_PASSWORD_LENGTH] = '\0';

        connect(manual_ssid, typed_ssid_password);  // initiates wifi connection
        current_state = STATE_CONNECTING;           // show connecting animation
        kb.reset_complete();
      }
      break;
  }
}
void WifiConfigScreen::connect(const char* ssid, const char* password) {
  WiFi.begin(ssid, password);
  connection_start_time = millis();
}

// drawing helper placeholders
void WifiConfigScreen::draw_signal_strength_bars(int32_t rssi, uint16_t x, uint16_t y, lgfx::LGFX_Device* lcd) {
  if (rssi < -70) {
    lcd->fillRect(x + 5, y + ITEM_HEIGHT / 2 - BARR_1_H, BARR_W, BARR_1_H, COLOR_RED);
    lcd->setColor(COLOR_BLACK);
    lcd->drawRect(x + 5 + BARR_2_X_OFFSET, y + ITEM_HEIGHT / 2 - BARR_2_H, BARR_W, BARR_2_H);
    lcd->drawRect(x + 5 + BARR_3_X_OFFSET, y + ITEM_HEIGHT / 2 - BARR_3_H, BARR_W, BARR_3_H);
    lcd->drawRect(x + 5 + BARR_4_X_OFFSET, y + ITEM_HEIGHT / 2 - BARR_4_H, BARR_W, BARR_4_H);
  } else if (rssi >= -70 && rssi < -55) {
    lcd->fillRect(x+5, y + ITEM_HEIGHT / 2 - BARR_1_H, BARR_W, BARR_1_H, COLOR_ORANGE);
    lcd->fillRect(x + 5 + BARR_2_X_OFFSET, y + ITEM_HEIGHT / 2 - BARR_2_H, BARR_W, BARR_2_H, COLOR_ORANGE);
    lcd->setColor(COLOR_BLACK);
    lcd->drawRect(x + 5 + BARR_3_X_OFFSET, y + ITEM_HEIGHT / 2 - BARR_3_H, BARR_W, BARR_3_H);
    lcd->drawRect(x + 5 + BARR_4_X_OFFSET, y + ITEM_HEIGHT / 2 - BARR_4_H, BARR_W, BARR_4_H);
  } else if (rssi >= -55 && rssi < -35) {
    lcd->fillRect(x+5, y + ITEM_HEIGHT / 2 - BARR_1_H, BARR_W, BARR_1_H, COLOR_DARKGREEN);
    lcd->fillRect(x + 5 + BARR_2_X_OFFSET, y + ITEM_HEIGHT / 2 - BARR_2_H, BARR_W, BARR_2_H, COLOR_DARKGREEN);
    lcd->fillRect(x + 5 + BARR_3_X_OFFSET, y + ITEM_HEIGHT / 2 - BARR_3_H, BARR_W, BARR_3_H, COLOR_DARKGREEN);
    lcd->setColor(COLOR_BLACK);
    lcd->drawRect(x + 5 + BARR_4_X_OFFSET, y + ITEM_HEIGHT / 2 - BARR_4_H, BARR_W, BARR_4_H);
  } else if (rssi >= -35) {
    lcd->fillRect(x+5, y + ITEM_HEIGHT / 2 - BARR_1_H, BARR_W, BARR_1_H, COLOR_GREEN);
    lcd->fillRect(x + 5 + BARR_2_X_OFFSET, y + ITEM_HEIGHT / 2 - BARR_2_H, BARR_W, BARR_2_H, COLOR_GREEN);
    lcd->fillRect(x + 5 + BARR_3_X_OFFSET, y + ITEM_HEIGHT / 2 - BARR_3_H, BARR_W, BARR_3_H, COLOR_GREEN);
    lcd->fillRect(x + 5 + BARR_4_X_OFFSET, y + ITEM_HEIGHT / 2 - BARR_4_H, BARR_W, BARR_4_H, COLOR_GREEN);
  }
}

void WifiConfigScreen::draw_message(uint16_t color,
                                    const char* message,
                                    bool animated,
                                    uint16_t x,
                                    uint16_t y,
                                    lgfx::LGFX_Device* lcd) {
                                      
  //clear screen                                 
  lcd->fillScreen(COLOR_WHITE);
  // draw message text
  lcd->setTextColor(color);
  lcd->setTextSize(KB_TEXT_SIZE_LARGE);
  lcd->setCursor(x, y);
  lcd->print(message);

  // draw animated dots if requested
  if (animated) {
    unsigned long current_time = millis();
    int dot_count = (current_time / 500) % 4;  // cycles 0-3 every 2 seconds

    lcd->fillRect(x, y + 20, 40, 16, COLOR_BLACK);  // clear rectangle for dots
    lcd->setCursor(x, y + 20);                      // position dots below message
    for (int i = 0; i < dot_count; i++) {
      lcd->print('.');
    }
  }
}

void WifiConfigScreen::draw_network_list(lgfx::LGFX_Device* lcd) {
  if (list_needs_redraw) {
    lcd->fillScreen(COLOR_WHITE);
    list_needs_redraw = false;
  }
  // draw header
  lcd->setTextColor(COLOR_BLACK);
  lcd->setTextSize(2);
  lcd->setCursor(10, 10);
  lcd->print("Select Network");

  // calculate which networks to show on this page
  int start_index = current_page * ITEMS_PER_PAGE;
  int end_index = min(start_index + ITEMS_PER_PAGE, (int)scanned_networks_amount);

  // draw each network item
  for (int i = start_index; i < end_index; i++) {
    int item_index = i - start_index;  // 0-4 for positioning
    uint16_t item_y = LIST_START_Y + (item_index * ITEM_HEIGHT);
    draw_network_list_item(scanned_networks[i], LIST_START_X, item_y, lcd);
  }

  // draw navigation buttons
  draw_bottom_buttons(lcd);
}

void WifiConfigScreen::draw_network_list_item(const network_info_t& network,
                                              uint16_t x,
                                              uint16_t y,
                                              lgfx::LGFX_Device* lcd) {
  lcd->setTextColor(COLOR_BLACK);
  lcd->setTextSize(2);
  lcd->setCursor(x, y);

  // check if ssid needs truncation
  int ssid_len = strlen(network.ssid);
  int max_chars = 20;  // fits on screen with room for bars

  if (ssid_len > max_chars) {
    // truncate and add ellipsis
    char truncated[max_chars + 4];  // +4 for "..." and '\0'
    strncpy(truncated, network.ssid, max_chars);
    truncated[max_chars] = '.';
    truncated[max_chars + 1] = '.';
    truncated[max_chars + 2] = '.';
    truncated[max_chars + 3] = '\0';
    lcd->print(truncated);
  } else {
    lcd->print(network.ssid);
  }

  // calculate bar position based on what was actually printed
  int displayed_len = (ssid_len > max_chars) ? max_chars + 3 : ssid_len;
  int bars_x = x + (displayed_len * 12);

  draw_signal_strength_bars(network.rssi, bars_x, y, lcd);
}

void WifiConfigScreen::draw_bottom_buttons(lgfx::LGFX_Device* lcd) {
  lcd->setTextColor(COLOR_BLACK);
  lcd->setColor(COLOR_BLACK);
    lcd->setTextSize(1);

  lcd->drawRect(PREV_BUTTON_X, BOTTOM_BUTTONS_Y, PREV_BUTTON_W, BUTTON_HEIGHT);
  lcd->setCursor(PREV_BUTTON_TEXT_X, BOTTOM_BUTTONS_TEXT_Y);
  lcd->print("Previous");

  lcd->drawRect(NEXT_BUTTON_X, BOTTOM_BUTTONS_Y, NEXT_BUTTON_W, BUTTON_HEIGHT);
  lcd->setCursor(NEXT_BUTTON_TEXT_X, BOTTOM_BUTTONS_TEXT_Y);
  lcd->print("Next");

  lcd->drawRect(RESCAN_BUTTON_X, BOTTOM_BUTTONS_Y, RESCAN_BUTTON_W, BUTTON_HEIGHT);
  lcd->setCursor(RESCAN_BUTTON_TEXT_X, BOTTOM_BUTTONS_TEXT_Y);
  lcd->print("Retry Scan");

  lcd->drawRect(MANUAL_BUTTON_X, BOTTOM_BUTTONS_Y, MANUAL_BUTTON_W, BUTTON_HEIGHT);
  lcd->setCursor(MANUAL_BUTTON_TEXT_X, BOTTOM_BUTTONS_TEXT_Y);
  lcd->print("Enter SSID");
}

bool WifiConfigScreen::is_point_in_rect(uint16_t touch_x,
                                        uint16_t touch_y,
                                        uint16_t rect_x,
                                        uint16_t rect_y,
                                        uint16_t rect_width,
                                        uint16_t rect_height) {
  return (touch_x >= rect_x && touch_x < rect_x + rect_width && touch_y >= rect_y && touch_y < rect_y + rect_height);
}