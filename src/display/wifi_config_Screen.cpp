// wifi_config_screen.cpp
#include "wifi_config_screen.h"


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
const uint16_t MANUAL_BUTTON_W = 76;                        // slightly wider to fill remaining space
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
  display_needs_redraw = true;  // redraw flag
  last_touch_time = 0;       // initialize debounce timer
}

// initiates async wifi network scan
void WifiConfigScreen::start_scan() {
  // Disconnect from any current connection attempt
  // Set WiFi to station mode (required for scanning)
  WiFi.mode(WIFI_STA);

  WiFi.disconnect(true);  // true = also clear saved AP settings in this session
  delay(100);             // Give the WiFi module time to fully disconnect

  WiFi.scanDelete();      // clean up any previous scan results
  selected_network = -1;  // reset selected network tracking
  display_needs_redraw = true;
  current_state = STATE_SCANNING;
  WiFi.scanNetworks(true, false);
}
// processes wifi scan results - sorts by signal strength and stores top 20
void WifiConfigScreen::process_scan_results(int networks_found) {
  networks_found = min(networks_found, 50);

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

  scanned_networks_amount = networks_to_copy;
}
// main draw method - renders current state
void WifiConfigScreen::draw(lgfx::LGFX_Device* lcd) {
  switch (current_state) {
    case STATE_SCANNING: {
      int scan_status = WiFi.scanComplete();

      if (scan_status == -2) {
        // scan failed
        Serial.println("wifi scan failed.");
        WiFi.scanDelete();
        display_needs_redraw = true;
        current_state = STATE_ERROR;
        state_change_time = millis();
      } else if (scan_status == -1) {
        // still scanning
        draw_message(COLOR_YELLOW, "Scanning networks", true, true, 120, 120, lcd);\
        draw_back_button(lcd);
      } else if (scan_status >= 0) {
        // scan completed - process results
        int networks_found = max(scan_status, 0);

        if (networks_found > 0) {
          process_scan_results(networks_found);
        } else {
          scanned_networks_amount = 0;
        }

        // cleanup and transition
        WiFi.scanDelete();
        display_needs_redraw = true;
        current_state = STATE_LIST;
      }
      break;
    }

    case STATE_LIST:
      draw_network_list_header(lcd);
      draw_network_list(lcd);
      draw_bottom_buttons(lcd);
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
        display_needs_redraw = true;
        current_state = STATE_SUCCESS;
        state_change_time = millis();
      } else if (status == WL_CONNECT_FAILED || status == WL_NO_SSID_AVAIL) {
        WiFi.disconnect(true);  // Clean up the failed connection
        display_needs_redraw = true;
        current_state = STATE_ERROR;
        state_change_time = millis();
      } else if (millis() - connection_start_time > WIFI_CONNECT_TIMEOUT_MS) {
        WiFi.disconnect(true);  // Clean up the failed connection
        display_needs_redraw = true;
        current_state = STATE_ERROR;
        state_change_time = millis();
      } else {
        // still connecting, draw animation
        draw_message(COLOR_YELLOW, "connecting", true, true, 180, 120, lcd);
        draw_network_list_header(lcd);
      }
      break;
    }

    case STATE_SUCCESS:
      draw_message(COLOR_GREEN, "Connected!", false, true, 120, 180, lcd);
      if (millis() - state_change_time > 1500) {
        display_needs_redraw = true;
        current_state = STATE_LIST;
      }
      break;

    case STATE_ERROR:
      // check if we came from scanning (no selected network) or from connecting
      if (selected_network == -1) {
        // error from scanning
        draw_message(COLOR_RED, "Scan failed!", false, true, 120, 180, lcd);
      } else {
        // error from connection attempt
        draw_message(COLOR_RED, "Error connecting!", false, true, 120, 180, lcd);
      }

      if (millis() - state_change_time > 5000) {
        display_needs_redraw = true;
        current_state = STATE_LIST;
      }
      draw_back_button(lcd);
      break;
  }
}

// placeholder method
void WifiConfigScreen::clear() {}

void WifiConfigScreen::handle_touch(uint16_t tx, uint16_t ty, lgfx::LGFX_Device* lcd) {
  // debounce
  unsigned long current_time = millis();
  if ((current_time - last_touch_time) < DEBOUNCE_DELAY) {
    return;
  }
  // update debounce timer
  last_touch_time = current_time;

  switch (current_state) {
    case STATE_SCANNING: {
      if (is_point_in_rect(tx, ty, BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_HEIGHT)) {
        WiFi.scanDelete();         // cancel ongoing scan
        display_needs_redraw = true;  // force redraw when returning to list
        current_state = STATE_LIST;
      }
      break;
    }
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
              display_needs_redraw = true;
              current_state = STATE_CONNECTING;
            } else {
              current_state = STATE_PASSWORD;
              kb.clear();
              kb.set_label("Enter network password");
              kb.set_max_length(MAX_WIFI_PASSWORD_LENGTH);
            }
            return;
          }
        }
      }

      // check button touches
      if (is_point_in_rect(tx, ty, PREV_BUTTON_X, BOTTOM_BUTTONS_Y, PREV_BUTTON_W, BUTTON_HEIGHT)) {
        if (current_page > 0) {
          display_needs_redraw = true;
          current_page--;
        }
      } else if (is_point_in_rect(tx, ty, NEXT_BUTTON_X, BOTTOM_BUTTONS_Y, NEXT_BUTTON_W, BUTTON_HEIGHT)) {
        int max_pages = (scanned_networks_amount + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE - 1;
        if (current_page < max_pages) {
          display_needs_redraw = true;
          current_page++;
        }
      } else if (is_point_in_rect(tx, ty, RESCAN_BUTTON_X, BOTTOM_BUTTONS_Y, RESCAN_BUTTON_W, BUTTON_HEIGHT)) {
        start_scan();

      } else if (is_point_in_rect(tx, ty, MANUAL_BUTTON_X, BOTTOM_BUTTONS_Y, MANUAL_BUTTON_W, BUTTON_HEIGHT)) {
        current_state = STATE_SSID_MANUAL_ENTRY;
        kb.clear();
        kb.set_label("Enter network ssid");
        kb.set_max_length(MAX_SSID_LENGTH);
      } else if (is_point_in_rect(tx, ty, BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_HEIGHT)) {
        // HERE WE WILL GET OUT OF THE WIFICONFIG INTERFACE
      }
      break;
    }
    case STATE_PASSWORD: {  // handle touch in password entering state
      kb.handle_touch(tx, ty, lcd);

      if (kb.is_cancelled()) {
        // user pressed back/cancel, return to network list
        display_needs_redraw = true;
        current_state = STATE_LIST;
        kb.clear();  // clears buffer and cancelled flag
      } else if (kb.is_complete()) {
        const char* password = kb.get_text();
        strncpy(typed_ssid_password, password, MAX_WIFI_PASSWORD_LENGTH);
        typed_ssid_password[MAX_WIFI_PASSWORD_LENGTH] = '\0';

        connect(scanned_networks[selected_network].ssid, typed_ssid_password);  // initiates wifi connection
        display_needs_redraw = true;
        current_state = STATE_CONNECTING;                                       // show connecting animation
        kb.reset_complete();
      }
      break;
    }

    case STATE_SSID_MANUAL_ENTRY: {
      kb.handle_touch(tx, ty, lcd);

      if (kb.is_cancelled()) {
        // user pressed back/cancel, return to network list
        display_needs_redraw = true;
        current_state = STATE_LIST;
        kb.clear();  // clears buffer and cancelled flag
      } else if (kb.is_complete()) {
        const char* ssid = kb.get_text();
        strncpy(manual_ssid, ssid, MAX_SSID_LENGTH);
        manual_ssid[MAX_SSID_LENGTH] = '\0';

        current_state = STATE_PASSWORD_MANUAL_ENTRY;
        kb.clear();  // show connecting animation
        kb.set_label("Enter network password");
        kb.set_max_length(MAX_WIFI_PASSWORD_LENGTH);
        kb.reset_complete();
      }
      break;
    }
    case STATE_PASSWORD_MANUAL_ENTRY: {
      kb.handle_touch(tx, ty, lcd);

      if (kb.is_cancelled()) {
        // user pressed back/cancel, go back to ssid entry
        current_state = STATE_SSID_MANUAL_ENTRY;
        kb.clear();
        kb.set_label("Enter network ssid");
        kb.set_max_length(MAX_SSID_LENGTH);
      } else if (kb.is_complete()) {
        const char* password = kb.get_text();
        strncpy(typed_ssid_password, password, MAX_WIFI_PASSWORD_LENGTH);
        typed_ssid_password[MAX_WIFI_PASSWORD_LENGTH] = '\0';

        connect(manual_ssid, typed_ssid_password);  // initiates wifi connection
        display_needs_redraw = true;
        current_state = STATE_CONNECTING;           // show connecting animation
        kb.reset_complete();
      }
      break;
    }

    case STATE_ERROR: {
      if (is_point_in_rect(tx, ty, BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_HEIGHT)) {
        display_needs_redraw = true;
        current_state = STATE_LIST;
      }
      break;
    }
  }
}
void WifiConfigScreen::connect(const char* ssid, const char* password) {
  WiFi.begin(ssid, password);
  connection_start_time = millis();
}

// drawing strength bars
void WifiConfigScreen::draw_signal_strength_bars(int32_t rssi,
                                                 uint16_t x,
                                                 uint16_t y,
                                                 bool small,
                                                 lgfx::LGFX_Device* lcd) {
  // scale factor for small bars
  float scale = small ? 0.7 : 1.0;

  // calculate scaled dimensions
  uint16_t bar_w = BARR_W * scale;
  uint16_t bar_1_h = BARR_1_H * scale;
  uint16_t bar_2_h = BARR_2_H * scale;
  uint16_t bar_3_h = BARR_3_H * scale;
  uint16_t bar_4_h = BARR_4_H * scale;
  uint16_t offset_2 = BARR_2_X_OFFSET * scale;
  uint16_t offset_3 = BARR_3_X_OFFSET * scale;
  uint16_t offset_4 = BARR_4_X_OFFSET * scale;

  // vertical reference point
  uint16_t y_ref = small ? y : (y + ITEM_HEIGHT / 2);

  if (rssi < -85) {
    lcd->setColor(COLOR_WHITE);
    lcd->drawRect(x + 5, y_ref - bar_1_h, bar_w, bar_1_h);
    lcd->drawRect(x + 5 + offset_2, y_ref - bar_2_h, bar_w, bar_2_h);
    lcd->drawRect(x + 5 + offset_3, y_ref - bar_3_h, bar_w, bar_3_h);
    lcd->drawRect(x + 5 + offset_4, y_ref - bar_4_h, bar_w, bar_4_h);
  } else if (rssi >= -85 && rssi < -75) {
    lcd->fillRect(x + 5, y_ref - bar_1_h, bar_w, bar_1_h, COLOR_RED);
    lcd->setColor(COLOR_WHITE);
    lcd->drawRect(x + 5 + offset_2, y_ref - bar_2_h, bar_w, bar_2_h);
    lcd->drawRect(x + 5 + offset_3, y_ref - bar_3_h, bar_w, bar_3_h);
    lcd->drawRect(x + 5 + offset_4, y_ref - bar_4_h, bar_w, bar_4_h);
  } else if (rssi >= -75 && rssi < -65) {
    lcd->fillRect(x + 5, y_ref - bar_1_h, bar_w, bar_1_h, COLOR_ORANGE);
    lcd->fillRect(x + 5 + offset_2, y_ref - bar_2_h, bar_w, bar_2_h, COLOR_ORANGE);
    lcd->setColor(COLOR_WHITE);
    lcd->drawRect(x + 5 + offset_3, y_ref - bar_3_h, bar_w, bar_3_h);
    lcd->drawRect(x + 5 + offset_4, y_ref - bar_4_h, bar_w, bar_4_h);
  } else if (rssi >= -65 && rssi < -55) {
    lcd->fillRect(x + 5, y_ref - bar_1_h, bar_w, bar_1_h, COLOR_DARKGREEN);
    lcd->fillRect(x + 5 + offset_2, y_ref - bar_2_h, bar_w, bar_2_h, COLOR_DARKGREEN);
    lcd->fillRect(x + 5 + offset_3, y_ref - bar_3_h, bar_w, bar_3_h, COLOR_DARKGREEN);
    lcd->setColor(COLOR_WHITE);
    lcd->drawRect(x + 5 + offset_4, y_ref - bar_4_h, bar_w, bar_4_h);
  } else if (rssi >= -45) {
    lcd->fillRect(x + 5, y_ref - bar_1_h, bar_w, bar_1_h, COLOR_GREEN);
    lcd->fillRect(x + 5 + offset_2, y_ref - bar_2_h, bar_w, bar_2_h, COLOR_GREEN);
    lcd->fillRect(x + 5 + offset_3, y_ref - bar_3_h, bar_w, bar_3_h, COLOR_GREEN);
    lcd->fillRect(x + 5 + offset_4, y_ref - bar_4_h, bar_w, bar_4_h, COLOR_GREEN);
  }
}

void WifiConfigScreen::draw_message(uint16_t color,
                                    const char* message,
                                    bool animated,
                                    bool is_centered,
                                    uint16_t x,
                                    uint16_t y,
                                    lgfx::LGFX_Device* lcd) {
  if (display_needs_redraw) {
    lcd->fillScreen(COLOR_BLACK);
    display_needs_redraw = false;
  }

  lcd->setTextSize(2);

  // calculate position
  int text_x = x;
  int text_y = y;

  if (is_centered) {
    // for centering, include dots in width calculation
    int message_width = strlen(message) * 12;
    int dots_width = animated ? (3 * 12) : 0;  // reserve space for max 3 dots
    int total_width = message_width + dots_width;
    int text_height = 16;

    text_x = (SCREEN_WIDTH - total_width) / 2;
    text_y = (SCREEN_HEIGHT - text_height) / 2;
  }

  // draw message text
  lcd->setTextColor(color);
  lcd->setCursor(text_x, text_y);
  lcd->print(message);

  // draw animated dots right after text on same line
  if (animated) {
    unsigned long current_time = millis();
    int dot_count = (current_time / 500) % 4;

    int message_width = strlen(message) * 12;
    int dots_x = text_x + message_width;

    // clear dot area (for all 3 dots)
    lcd->fillRect(dots_x, text_y, 36, 16, COLOR_BLACK);

    // draw current dots
    lcd->setCursor(dots_x, text_y);
    for (int i = 0; i < dot_count; i++) {
      lcd->print('.');
    }
  }
}
void WifiConfigScreen::draw_network_list_header(lgfx::LGFX_Device* lcd) {
  // back button
  draw_back_button(lcd);

  // draw header text
  int header_text_length = strlen("Select Network");
  lcd->setTextColor(COLOR_WHITE);
  lcd->setTextSize(2);
  lcd->setCursor(HEADER_TEXT_X, HEADER_TEXT_Y);
  lcd->print("Select Network");

  // show network connected to
  lcd->setTextColor(COLOR_WHITE);
  lcd->setTextSize(1);

  if (WiFi.status() == WL_CONNECTED) {
    const int right_margin = 5;            // pixels from right edge
    const int bars_width = 10;             // total width of signal bars
    const int text_spacing = 1;            // gap between text and bars
    const int text_to_network_gap = 20;    // gap between header text and network info
    const int bars_internal_offset = 5;    // built-in offset in draw_signal_strength_bars

    
    // position bars from right edge
    int bars_x = SCREEN_WIDTH - right_margin - bars_width - bars_internal_offset;

    
    // calculate where text can end
    int text_max_x = bars_x - text_spacing;
    
    // calculate available text width
    int text_start_x = HEADER_TEXT_X + (header_text_length * 12) + text_to_network_gap;
    int available_text_width = text_max_x - text_start_x;
    int max_chars = available_text_width / 6;  // 6 pixels per character at text size 1
    
    // get ssid and truncate if needed
    int ssid_len = WiFi.SSID().length();
    char display_text[32];
    
    if (ssid_len > max_chars && max_chars > 3) {
      // truncate and add ellipsis
      strncpy(display_text, WiFi.SSID().c_str(), max_chars - 3);
      display_text[max_chars - 3] = '\0';
      strcat(display_text, "...");
    } else {
      strncpy(display_text, WiFi.SSID().c_str(), sizeof(display_text) - 1);
      display_text[sizeof(display_text) - 1] = '\0';
    }
    
    // position text to end at text_max_x
    int actual_text_len = strlen(display_text);
    int text_x = text_max_x - (actual_text_len * 6);
    
    lcd->setCursor(text_x, HEADER_NETWORK_Y);
    lcd->print(display_text);
    
    draw_signal_strength_bars(WiFi.RSSI(), bars_x , HEADER_NETWORK_Y + 7, true, lcd);

  } else {
    const int right_margin = 5;
    int no_conn_len = strlen("No connection");
    lcd->setCursor(SCREEN_WIDTH - right_margin - (no_conn_len * 6) - 10, HEADER_NETWORK_Y);
    lcd->fillRect(SCREEN_WIDTH - right_margin - (no_conn_len * 6) - 10, HEADER_NETWORK_Y,(no_conn_len * 6), HEADER_NETWORK_Y, COLOR_BLACK);
    lcd->print("No connection");
  }
}
void WifiConfigScreen::draw_network_list(lgfx::LGFX_Device* lcd) {
  if (display_needs_redraw) {
    lcd->fillScreen(COLOR_BLACK);
    display_needs_redraw = false;
  }

  // check if we have networks to display
  if (scanned_networks_amount == 0) {
    lcd->setTextSize(1);
    lcd->setTextColor(COLOR_YELLOW);
    lcd->setCursor(80, 100);
    lcd->print("No networks available");
    lcd->setCursor(70, 115);
    lcd->print("Press 'Retry Scan' to scan");

  } else {
    // draw network items
    int start_index = current_page * ITEMS_PER_PAGE;
    int end_index = min(start_index + ITEMS_PER_PAGE, (int)scanned_networks_amount);

    for (int i = start_index; i < end_index; i++) {
      int item_index = i - start_index;
      uint16_t item_y = LIST_START_Y + (item_index * (ITEM_HEIGHT + ITEM_GAP));
      draw_network_list_item(scanned_networks[i], LIST_START_X, item_y, lcd);
    }
  }
}

void WifiConfigScreen::draw_network_list_item(const network_info_t& network,
                                              uint16_t x,
                                              uint16_t y,
                                              lgfx::LGFX_Device* lcd) {
  // draw rectangle around item
  lcd->setColor(COLOR_WHITE);
  lcd->drawRect(x, y, SCREEN_WIDTH - (2 * x), ITEM_HEIGHT, COLOR_WHITE);

  // draw text inside
  lcd->setTextColor(COLOR_WHITE);
  lcd->setTextSize(2);
  lcd->setCursor(x + 3, y + 12);  // offset from border

  // check if ssid needs truncation
  int ssid_len = strlen(network.ssid);
  int max_chars = 15;  // reduce slightly to fit inside rectangle

  if (ssid_len > max_chars) {
    char truncated[max_chars + 4];
    strncpy(truncated, network.ssid, max_chars);
    truncated[max_chars] = '.';
    truncated[max_chars + 1] = '.';
    truncated[max_chars + 2] = '.';
    truncated[max_chars + 3] = '\0';
    lcd->print(truncated);
  } else {
    lcd->print(network.ssid);
  }

  int displayed_len = (ssid_len > max_chars) ? max_chars + 3 : ssid_len;
  int bars_x = x + (displayed_len * 12);

  draw_signal_strength_bars(network.rssi, bars_x, y + 6, false, lcd);
}
void WifiConfigScreen::draw_back_button(lgfx::LGFX_Device* lcd) {
  lcd->setTextColor(COLOR_WHITE);
  lcd->setColor(COLOR_WHITE);
  lcd->setTextSize(2);

  lcd->drawRect(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_HEIGHT);
  lcd->setCursor(BACK_BUTTON_TEXT_X, BACK_BUTTON_TEXT_Y);
  lcd->print("<-");
}

void WifiConfigScreen::draw_bottom_buttons(lgfx::LGFX_Device* lcd) {
  lcd->setTextColor(COLOR_WHITE);
  lcd->setColor(COLOR_WHITE);
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