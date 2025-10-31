// main.cpp: wifi config screen test
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <WiFi.h>
#include "config.h"
#include "display/wifi_config_screen.h"

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
      
      cfg.panel_width = SCREEN_HEIGHT;   // 240
      cfg.panel_height = SCREEN_WIDTH;   // 320
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

      cfg.invert = true;        // Enable color inversion
      //cfg.rgb_order = false;

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
      cfg.offset_rotation = 0;
      
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

// global instances
LGFX lcd;
WifiConfigScreen wifi_screen;

// setup
void setup() {
  // init serial
  Serial.begin(SERIAL_BAUD_RATE);
  delay(1000);
  Serial.println("\n================================");
  Serial.println("   wifi config screen test");
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
  WiFi.disconnect();
  delay(100);
  
  Serial.println("[ok] wifi initialized");
  
  // start network scan
  wifi_screen.start_scan();
  Serial.println("[info] starting wifi scan...");
  Serial.println();
}

// main loop
void loop() {
  uint16_t tx, ty;
  
  // check for touch
  if (lcd.getTouch(&tx, &ty)) {
    // invert coordinates for landscape rotation
    uint16_t x = lcd.width() - 1 - tx;
    uint16_t y = lcd.height() - 1 - ty;
    
    // pass touch to wifi screen handler
    wifi_screen.handle_touch(x, y, &lcd);
  }
  
  // draw current wifi screen state
  wifi_screen.draw(&lcd);
  
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
}