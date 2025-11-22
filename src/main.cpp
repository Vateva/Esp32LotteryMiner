// main.cpp: complete ui test with navigation
#define LGFX_USE_V1
#include <WiFi.h>

#include <LovyanGFX.hpp>

#include "config.h"
#include "main_menu.h"
#include "pool_config_screen.h"
#include "wallet_config_screen.h"
#include "wifi_config_screen.h"

// lgfx class for ili9341 + ft6336 touch
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ILI9341 _panel_instance;
  lgfx::Bus_SPI _bus_instance;
  lgfx::Touch_FT5x06 _touch_instance;

 public:
  LGFX(void) {
    // spi bus config
    {
      auto cfg = _bus_instance.config();
      cfg.spi_host = SPI2_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = SPI_FREQUENCY_WRITE;
      cfg.freq_read = SPI_FREQUENCY_READ;
      cfg.spi_3wire = false;
      cfg.use_lock = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;

      cfg.pin_sclk = PIN_LCD_SCLK;
      cfg.pin_mosi = PIN_LCD_MOSI;
      cfg.pin_miso = PIN_LCD_MISO;
      cfg.pin_dc = PIN_LCD_DC;

      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }

    // panel config
    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs = PIN_LCD_CS;
      cfg.pin_rst = PIN_LCD_RST;
      cfg.pin_busy = -1;

      cfg.panel_width = SCREEN_HEIGHT;  // 240
      cfg.panel_height = SCREEN_WIDTH;  // 320
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.offset_rotation = 0;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits = 1;
      cfg.readable = true;
      cfg.invert = false;
      cfg.rgb_order = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = true;

      cfg.invert = true;  // enable color inversion

      _panel_instance.config(cfg);
    }

    // touch panel config (ft6336)
    {
      auto cfg = _touch_instance.config();
      cfg.x_min = TOUCH_X_MIN;
      cfg.x_max = TOUCH_X_MAX;
      cfg.y_min = TOUCH_Y_MIN;
      cfg.y_max = TOUCH_Y_MAX;
      cfg.pin_int = PIN_TOUCH_INT;
      cfg.pin_rst = -1;
      cfg.bus_shared = false;
      cfg.offset_rotation = 2;

      cfg.i2c_port = 0;
      cfg.i2c_addr = TOUCH_I2C_ADDR;
      cfg.freq = TOUCH_I2C_FREQUENCY;
      cfg.pin_sda = PIN_TOUCH_SDA;
      cfg.pin_scl = PIN_TOUCH_SCL;

      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);
    }

    setPanel(&_panel_instance);
  }
};

// screen navigation states - tracks which screen is currently active
enum app_screen_t {
  SCREEN_HOME,           // home screen with main menu button
  SCREEN_MAIN_MENU,      // main menu with 4 options
  SCREEN_WIFI_CONFIG,    // wifi configuration
  SCREEN_WALLET_CONFIG,  // wallet configuration
  SCREEN_POOL_CONFIG,    // pool configuration
  SCREEN_THEMES          // placeholder for mining screen
};

// global instances
LGFX lcd;
MainMenu main_menu;
WifiConfigScreen wifi_screen;
WalletConfigScreen wallet_screen;
PoolConfigScreen pool_screen;

// navigation state
app_screen_t current_screen = SCREEN_HOME;
app_screen_t previous_screen = SCREEN_HOME;
bool screen_needs_redraw = true;  // flag to trigger full screen clear when changing screens

// home screen button coordinates
#define HOME_BUTTON_X 85
#define HOME_BUTTON_Y 100
#define HOME_BUTTON_WIDTH 150
#define HOME_BUTTON_HEIGHT 40

// draw home screen with centered text and main menu button
void draw_home_screen() {
  // only redraw when needed to prevent flickering
  if (screen_needs_redraw) {
    lcd.fillScreen(COLOR_BLACK);
    screen_needs_redraw = false;

    // draw placeholder text centered at top
    lcd.setTextColor(COLOR_WHITE);
    lcd.setTextSize(2);
    lcd.setCursor(40, 40);
    lcd.print("Placeholder Home");
    lcd.setCursor(85, 60);
    lcd.print("Page");

    // draw main menu button
    lcd.drawRect(HOME_BUTTON_X, HOME_BUTTON_Y, HOME_BUTTON_WIDTH, HOME_BUTTON_HEIGHT, COLOR_WHITE);
    lcd.setCursor(HOME_BUTTON_X + 20, HOME_BUTTON_Y + 12);
    lcd.print("Main Menu");
  }
}

// handle touch input on home screen
void handle_home_touch(uint16_t tx, uint16_t ty) {
  // check if main menu button was touched
  if (tx >= HOME_BUTTON_X && tx < HOME_BUTTON_X + HOME_BUTTON_WIDTH && ty >= HOME_BUTTON_Y &&
      ty < HOME_BUTTON_Y + HOME_BUTTON_HEIGHT) {
    // navigate to main menu
    previous_screen = current_screen;
    current_screen = SCREEN_MAIN_MENU;
    main_menu.mark_for_redraw();
    screen_needs_redraw = true;

    Serial.println("[nav] home -> main menu");
  }
}

// setup
void setup() {
  // init serial
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n================================");
  Serial.println("   complete ui test");
  Serial.println("================================");

  // init display
  lcd.init();
  lcd.setRotation(1);  // landscape
  lcd.fillScreen(COLOR_BLACK);

  Serial.println("[ok] display initialized");

  // check touch controller
  if (lcd.touch()) {
    Serial.println("[ok] touch controller ready");
  } else {
    Serial.println("[error] touch controller not detected!");
  }

  // set wifi mode to station
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();     // clear any previous connection state
  WiFi.setSleep(false);  // disable wifi sleep - critical for esp32-s3 scanning
  delay(500);            // give radio time to fully stabilize

  Serial.println("[ok] wifi initialized");

  // load saved nvs parameters
  wifi_screen.load_from_nvs();
  wallet_screen.load_from_nvs();
  pool_screen.load_from_nvs();

  Serial.println("[ok] all screens initialized");
  Serial.println("[info] starting on home screen");
  Serial.println();
}

// main loop
void loop() {
  uint16_t x = 0, y = 0;

  // check for touch input
  bool touched = lcd.getTouch(&x, &y);

  // handle current screen based on navigation state
  switch (current_screen) {
    case SCREEN_HOME:
      // draw home screen
      draw_home_screen();

      // handle touch if detected
      if (touched) {
        handle_home_touch(x, y);
      }
      break;

    case SCREEN_MAIN_MENU:
      // draw main menu
      main_menu.draw(&lcd);

      // handle touch if detected
      if (touched) {
        main_menu.handle_touch(x, y, &lcd);

        // check if user selected a menu item
        int8_t selected = main_menu.get_selected_item();
        if (selected >= 0) {
          // navigate based on selection
          previous_screen = current_screen;

          switch (selected) {
            case 0:
              // wallets
              current_screen = SCREEN_WALLET_CONFIG;
              wallet_screen.mark_for_redraw();
              screen_needs_redraw = true;
              Serial.println("[nav] main menu -> wallets");
              break;

            case 1:
              // pools
              current_screen = SCREEN_POOL_CONFIG;
              pool_screen.mark_for_redraw();
              screen_needs_redraw = true;
              Serial.println("[nav] main menu -> pools");
              break;

            case 2:
              // wifi
              current_screen = SCREEN_WIFI_CONFIG;
              wifi_screen.start_scan();
              screen_needs_redraw = true;
              Serial.println("[nav] main menu -> wifi");
              break;

            case 3:
              // themes - placeholder
              current_screen = SCREEN_THEMES;
              // themes.mark_for_redraw();
              screen_needs_redraw = true;
              Serial.println("[nav] main menu -> ");
              break;

            default:
              break;
          }

          // clear selection after navigation
          main_menu.reset_selection();
        }
      }
      break;

    case SCREEN_WIFI_CONFIG:
      // draw wifi config screen
      wifi_screen.draw(&lcd);

      // handle touch if detected
      if (touched) {
        wifi_screen.handle_touch(x, y, &lcd);
      }

      // check if connected
      if (WiFi.status() == WL_CONNECTED) {
        // print connection info once
        static bool info_printed = false;
        if (!info_printed) {
          Serial.println("\n[success] wifi connected!");
          Serial.print("  ssid: ");
          Serial.println(WiFi.SSID());
          Serial.print("  ip address: ");
          Serial.println(WiFi.localIP());
          Serial.print("  rssi: ");
          Serial.print(WiFi.RSSI());
          Serial.println(" dBm");
          info_printed = true;
        }
      }
      break;

    case SCREEN_WALLET_CONFIG:
      // draw wallet config screen
      wallet_screen.draw(&lcd);

      // handle touch if detected
      if (touched) {
        wallet_screen.handle_touch(x, y, &lcd);
      }
      break;

    case SCREEN_POOL_CONFIG:
      // draw pool config screen
      pool_screen.draw(&lcd);

      // handle touch if detected
      if (touched) {
        pool_screen.handle_touch(x, y, &lcd);
      }
      break;

    case SCREEN_THEMES:
      // placeholder mining screen
      if (screen_needs_redraw) {
        lcd.fillScreen(COLOR_BLACK);
        screen_needs_redraw = false;

        // draw placeholder text
        lcd.setTextColor(COLOR_WHITE);
        lcd.setTextSize(2);
        lcd.setCursor(60, 100);
        lcd.print("Themes Screen");
        lcd.setCursor(70, 120);
        lcd.print("Placeholder");

        // draw back button
        lcd.drawRect(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_W, BACK_BUTTON_HEIGHT, COLOR_WHITE);
        lcd.setCursor(BACK_BUTTON_TEXT_X, BACK_BUTTON_TEXT_Y);
        lcd.print("<-");
      }

      // handle touch for back button
      if (touched) {
        if (x >= BACK_BUTTON_X && x < BACK_BUTTON_X + BACK_BUTTON_W && y >= BACK_BUTTON_Y &&
            y < BACK_BUTTON_Y + BACK_BUTTON_HEIGHT) {
          // return to main menu
          current_screen = SCREEN_MAIN_MENU;
          screen_needs_redraw = true;
          Serial.println("[nav] mining -> main menu");
        }
      }
      break;
  }

  // universal back button handling for config screens
  // this checks if back button was touched while in any config screen and returns to main menu
  if (current_screen == SCREEN_WIFI_CONFIG || current_screen == SCREEN_WALLET_CONFIG ||
      current_screen == SCREEN_POOL_CONFIG) {
    if (touched) {
      // check if back button area was touched
      if (x >= BACK_BUTTON_X && x < BACK_BUTTON_X + BACK_BUTTON_W && y >= BACK_BUTTON_Y &&
          y < BACK_BUTTON_Y + BACK_BUTTON_HEIGHT) {
        // return to main menu
        current_screen = SCREEN_MAIN_MENU;
        main_menu.mark_for_redraw();
        screen_needs_redraw = true;
        Serial.println("[nav] config screen -> main menu");
      }
    }
  }
}