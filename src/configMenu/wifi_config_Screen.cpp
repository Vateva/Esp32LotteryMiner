// wifi_config_screen.cpp
#include "configMenu/wifi_config_screen.h"
#include "configMenu/ui_utils.h"

#include <Preferences.h>


// buttons at bottom
const uint16_t BOTTOM_BUTTONS_Y = 209;
const uint16_t BOTTOM_BUTTONS_TEXT_Y = BOTTOM_BUTTONS_Y + 10;  // 219
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

// ----------------------------------------------------------------------------
// Constructor
// ----------------------------------------------------------------------------

// constructor - initialize screen state
WifiConfigScreen::WifiConfigScreen() {
  current_state = wifi_screen_state_t::STATE_LIST;
  current_page = 0;
  selected_network = -1;
  scanned_networks_amount = 0;
  mark_for_redraw();  // redraw flag
  saved_ssid[0] = '\0';
  saved_password[0] = '\0';
}

// ----------------------------------------------------------------------------
// Public Interface Functions (Main Loop)
// ----------------------------------------------------------------------------

// main draw method - renders current state
void WifiConfigScreen::draw(lgfx::LGFX_Device* lcd) {
  switch (current_state) {
    case wifi_screen_state_t::STATE_SCANNING: {
      if (display_needs_redraw) {
        WiFi.scanNetworks(true, false);  // Start the non-blocking scan
      }
      int scan_status = WiFi.scanComplete();

      if (scan_status == -2) {
        // scan failed
        Serial.println("wifi scan failed.");
        WiFi.scanDelete();
        mark_for_redraw();
        strcpy(error_message, "Scan failed!");
        current_state = wifi_screen_state_t::STATE_ERROR;
        state_change_time = millis();
      } else if (scan_status == -1) {
        // still scanning
        draw_message(COLOR_YELLOW, "Scanning networks", true, true, 120, 120, lcd);
        UIUtils::draw_back_button(lcd);
      } else if (scan_status >= 0) {
        // scan completed - process results
        int networks_found = max(scan_status, 0);
        bool is_autoconnecting = false;  // flag to track autoconnect status

        if (networks_found > 0) {
          // Check the return value to see if auto-connect started
          is_autoconnecting = process_scan_results(networks_found);
        } else {
          scanned_networks_amount = 0;
        }

        // cleanup and transition
        WiFi.scanDelete();
        mark_for_redraw();

        // only go to STATE_LIST if we are NOT auto-connecting
        if (!is_autoconnecting) {
          current_state = wifi_screen_state_t::STATE_LIST;
        }
        // if auto-connecting, we do nothing,
        // leaving the state as wifi_screen_state_t::STATE_CONNECTING
      }
      break;
    }

    case wifi_screen_state_t::STATE_LIST:
      draw_network_list_header(lcd);
      draw_network_list(lcd);
      draw_bottom_buttons(lcd);
      break;

    case wifi_screen_state_t::STATE_PASSWORD:
      kb.draw(lcd);
      break;

    case wifi_screen_state_t::STATE_SSID_MANUAL_ENTRY:
      kb.draw(lcd);
      break;

    case wifi_screen_state_t::STATE_PASSWORD_MANUAL_ENTRY:
      kb.draw(lcd);
      break;

    case wifi_screen_state_t::STATE_CONNECTING: {
      int status = WiFi.status();

      if (status == WL_CONNECTED) {
        mark_for_redraw();
        current_state = wifi_screen_state_t::STATE_SUCCESS;
        state_change_time = millis();
      } else if (status == WL_CONNECT_FAILED || status == WL_NO_SSID_AVAIL) {
        WiFi.disconnect(true);  // Clean up the failed connection
        mark_for_redraw();
        strcpy(error_message, "Error connecting!");
        current_state = wifi_screen_state_t::STATE_ERROR;
        state_change_time = millis();
      } else if (millis() - connection_start_time > WIFI_CONNECT_TIMEOUT_MS) {  // ADD THIS
        WiFi.disconnect(true);
        mark_for_redraw();
        strcpy(error_message, "Connection timeout!");
        current_state = wifi_screen_state_t::STATE_ERROR;
        state_change_time = millis();
      } else {
        // still connecting, draw animation
        draw_message(COLOR_YELLOW, "connecting", true, true, 180, 120, lcd);
        draw_network_list_header(lcd);
      }
      break;
    }

    case wifi_screen_state_t::STATE_SUCCESS:
      save_to_nvs();
      draw_message(COLOR_GREEN, "Connected!", false, true, 120, 180, lcd);
      if (millis() - state_change_time > 1500) {
        mark_for_redraw();
        current_state = wifi_screen_state_t::STATE_LIST;
      }
      break;

    case wifi_screen_state_t::STATE_ERROR:

      draw_message(COLOR_RED, error_message, false, true, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 10, lcd);

      if (millis() - state_change_time > 5000) {
        mark_for_redraw();
        current_state = wifi_screen_state_t::STATE_LIST;
      }
      UIUtils::draw_back_button(lcd);
      break;
  }
}

void WifiConfigScreen::handle_touch(uint16_t tx, uint16_t ty, lgfx::LGFX_Device* lcd) {
  // debounce touch input
  if (!UIUtils::touch_debounce()) {
    return;
  }
  switch (current_state) {
    case wifi_screen_state_t::STATE_SCANNING: {
      if (UIUtils::is_point_in_rect(tx, ty, BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_HEIGHT)) {
        WiFi.scanDelete();  // cancel ongoing scan
        mark_for_redraw();  // force redraw when returning to list
        current_state = wifi_screen_state_t::STATE_LIST;
      }
      break;
    }
    case wifi_screen_state_t::STATE_LIST: {
      // check network item touches
      for (int i = 0; i < LIST_SLOTS; i++) {
        uint16_t item_y = LIST_START_Y + (ITEM_HEIGHT * i);
        if (UIUtils::is_point_in_rect(tx, ty, LIST_START_X, item_y, SCREEN_WIDTH, ITEM_HEIGHT)) {
          int network_index = (current_page * LIST_SLOTS) + i;

          if (network_index < scanned_networks_amount) {
            selected_network = network_index;

            if (scanned_networks[network_index].encryption == WIFI_AUTH_OPEN) {
              connect(scanned_networks[selected_network].ssid, "");
              mark_for_redraw();
              current_state = wifi_screen_state_t::STATE_CONNECTING;
            } else {
              current_state = wifi_screen_state_t::STATE_PASSWORD;
              kb.clear();
              kb.set_label("Enter network password");
              kb.set_max_length(MAX_WIFI_PASSWORD_LENGTH);
            }
            return;
          }
        }
      }

      // check button touches
      if (UIUtils::is_point_in_rect(tx, ty, PREV_BUTTON_X, BOTTOM_BUTTONS_Y, PREV_BUTTON_W, BUTTON_HEIGHT)) {
        if (current_page > 0) {
          mark_for_redraw();
          current_page--;
        }
      } else if (UIUtils::is_point_in_rect(tx, ty, NEXT_BUTTON_X, BOTTOM_BUTTONS_Y, NEXT_BUTTON_W, BUTTON_HEIGHT)) {
        int max_pages = (scanned_networks_amount + LIST_SLOTS - 1) / LIST_SLOTS - 1;
        if (current_page < max_pages) {
          mark_for_redraw();
          current_page++;
        }
      } else if (UIUtils::is_point_in_rect(tx, ty, RESCAN_BUTTON_X, BOTTOM_BUTTONS_Y, RESCAN_BUTTON_W, BUTTON_HEIGHT)) {
        start_scan();

      } else if (UIUtils::is_point_in_rect(tx, ty, MANUAL_BUTTON_X, BOTTOM_BUTTONS_Y, MANUAL_BUTTON_W, BUTTON_HEIGHT)) {
        current_state = wifi_screen_state_t::STATE_SSID_MANUAL_ENTRY;
        kb.clear();
        kb.set_label("Enter network ssid");
        kb.set_max_length(MAX_SSID_LENGTH);
      } else if (UIUtils::is_point_in_rect(tx, ty, BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_HEIGHT)) {
        // HERE WE WILL GET OUT OF THE WIFICONFIG INTERFACE
      }
      break;
    }
    case wifi_screen_state_t::STATE_PASSWORD: {  // handle touch in password entering state
      kb.handle_touch(tx, ty, lcd);

      if (kb.is_cancelled()) {
        // user pressed back/cancel, return to network list
        mark_for_redraw();
        current_state = wifi_screen_state_t::STATE_LIST;
        kb.clear();  // clears buffer and cancelled flag
      } else if (kb.is_complete()) {
        const char* password = kb.get_text();
        strncpy(typed_ssid_password, password, MAX_WIFI_PASSWORD_LENGTH);
        typed_ssid_password[MAX_WIFI_PASSWORD_LENGTH] = '\0';

        connect(scanned_networks[selected_network].ssid, typed_ssid_password);  // initiates wifi connection
        mark_for_redraw();
        current_state = wifi_screen_state_t::STATE_CONNECTING;  // show connecting animation
        kb.reset_complete();
      }
      break;
    }

    case wifi_screen_state_t::STATE_SSID_MANUAL_ENTRY: {
      kb.handle_touch(tx, ty, lcd);

      if (kb.is_cancelled()) {
        // user pressed back/cancel, return to network list
        mark_for_redraw();
        current_state = wifi_screen_state_t::STATE_LIST;
        kb.clear();  // clears buffer and cancelled flag
      } else if (kb.is_complete()) {
        const char* ssid = kb.get_text();
        strncpy(manual_ssid, ssid, MAX_SSID_LENGTH);
        manual_ssid[MAX_SSID_LENGTH] = '\0';

        current_state = wifi_screen_state_t::STATE_PASSWORD_MANUAL_ENTRY;
        kb.clear();  // show connecting animation
        kb.set_label("Enter network password");
        kb.set_max_length(MAX_WIFI_PASSWORD_LENGTH);
        kb.reset_complete();
      }
      break;
    }
    case wifi_screen_state_t::STATE_PASSWORD_MANUAL_ENTRY: {
      kb.handle_touch(tx, ty, lcd);

      if (kb.is_cancelled()) {
        // user pressed back/cancel, go back to ssid entry
        current_state = wifi_screen_state_t::STATE_SSID_MANUAL_ENTRY;
        kb.clear();
        kb.set_label("Enter network ssid");
        kb.set_max_length(MAX_SSID_LENGTH);
      } else if (kb.is_complete()) {
        const char* password = kb.get_text();
        strncpy(typed_ssid_password, password, MAX_WIFI_PASSWORD_LENGTH);
        typed_ssid_password[MAX_WIFI_PASSWORD_LENGTH] = '\0';

        connect(manual_ssid, typed_ssid_password);  // initiates wifi connection
        mark_for_redraw();
        current_state = wifi_screen_state_t::STATE_CONNECTING;  // show connecting animation
        kb.reset_complete();
      }
      break;
    }

    case wifi_screen_state_t::STATE_ERROR: {
      if (UIUtils::is_point_in_rect(tx, ty, BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_HEIGHT)) {
        mark_for_redraw();
        current_state = wifi_screen_state_t::STATE_LIST;
      }
      break;
    }
  }
}

// initiates async wifi network scan
void WifiConfigScreen::start_scan() {
  mark_for_redraw();
  current_state = wifi_screen_state_t::STATE_SCANNING;
}

// placeholder method
void WifiConfigScreen::clear() {}

// ----------------------------------------------------------------------------
// Internal Logic (WiFi, State Management)
// ----------------------------------------------------------------------------

void WifiConfigScreen::connect(const char* ssid, const char* password) {
  // store what we're connecting with
  strncpy(saved_ssid, ssid, MAX_SSID_LENGTH);
  saved_ssid[MAX_SSID_LENGTH] = '\0';

  strncpy(saved_password, password, MAX_WIFI_PASSWORD_LENGTH);
  saved_password[MAX_WIFI_PASSWORD_LENGTH] = '\0';

  WiFi.begin(ssid, password);
  connection_start_time = millis();
}

bool WifiConfigScreen::try_auto_connect() {
  if (strlen(saved_ssid) > 0) {
    for (int i = 0; i < scanned_networks_amount; i++) {
      if (strcmp(scanned_networks[i].ssid, saved_ssid) == 0) {
        current_state = wifi_screen_state_t::STATE_CONNECTING;  // show connecting animation
        connect(saved_ssid, saved_password);
        mark_for_redraw();

        return true;  // found network to connect to
      }
    }
  }
  return false;
};

// processes wifi scan results - sorts by signal strength and stores top 20
bool WifiConfigScreen::process_scan_results(int networks_found) {
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

  return try_auto_connect();
}

// ----------------------------------------------------------------------------
// Drawing Helper Functions
// ----------------------------------------------------------------------------

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
  UIUtils::draw_back_button(lcd);

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
    const int right_margin = 5;          // pixels from right edge
    const int bars_width = 10;           // total width of signal bars
    const int text_spacing = 1;          // gap between text and bars
    const int text_to_network_gap = 20;  // gap between header text and network info
    const int bars_internal_offset = 5;  // built-in offset in draw_signal_strength_bars

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

    draw_signal_strength_bars(WiFi.RSSI(), bars_x, HEADER_NETWORK_Y + 7, true, lcd);

  } else {
    const int right_margin = 5;
    int no_conn_len = strlen("No connection");
    lcd->setCursor(SCREEN_WIDTH - right_margin - (no_conn_len * 6) - 10, HEADER_NETWORK_Y);
    lcd->fillRect(SCREEN_WIDTH - right_margin - (no_conn_len * 6) - 10,
                  HEADER_NETWORK_Y,
                  (no_conn_len * 6),
                  HEADER_NETWORK_Y,
                  COLOR_BLACK);
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
    int start_index = current_page * LIST_SLOTS;
    int end_index = min(start_index + (int)LIST_SLOTS, (int)scanned_networks_amount);

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
  } else if (rssi >= -55) {
    lcd->fillRect(x + 5, y_ref - bar_1_h, bar_w, bar_1_h, COLOR_GREEN);
    lcd->fillRect(x + 5 + offset_2, y_ref - bar_2_h, bar_w, bar_2_h, COLOR_GREEN);
    lcd->fillRect(x + 5 + offset_3, y_ref - bar_3_h, bar_w, bar_3_h, COLOR_GREEN);
    lcd->fillRect(x + 5 + offset_4, y_ref - bar_4_h, bar_w, bar_4_h, COLOR_GREEN);
  }
}

void WifiConfigScreen::mark_for_redraw() {
  display_needs_redraw = true;
}

// ----------------------------------------------------------------------------
// Persistence (NVS)
// ----------------------------------------------------------------------------

// nvs persistence
void WifiConfigScreen::save_to_nvs() {
  Preferences prefs;
  prefs.begin("esp32btcminer", false);  // open namespace in read/write mode

  prefs.putString("wifi_ssid", saved_ssid);
  prefs.putString("wifi_pass", saved_password);

  prefs.end();  // close namespace
}
void WifiConfigScreen::load_from_nvs() {
  Preferences prefs;
  prefs.begin("esp32btcminer", true);  // open namespace in read mode

  // load empty strings as default if nothing stored
  String ssid = prefs.getString("wifi_ssid", "");
  String password = prefs.getString("wifi_pass", "");

  // copy strings to saved_ssid and saved_password variables
  strncpy(saved_ssid, ssid.c_str(), MAX_SSID_LENGTH);
  saved_ssid[MAX_SSID_LENGTH] = '\0';

  strncpy(saved_password, password.c_str(), MAX_WIFI_PASSWORD_LENGTH);
  saved_password[MAX_WIFI_PASSWORD_LENGTH] = '\0';

  prefs.end();
}
