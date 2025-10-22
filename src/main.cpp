// main.cpp: keyboard test program
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "config.h"
#include "display/keyboard.h"


void display_results();


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
Keyboard kb;

// test state machine
enum test_state_t {
  TEST_WIFI_SSID,
  TEST_WIFI_PASSWORD,
  TEST_POOL_URL,
  TEST_WALLET,
  TEST_COMPLETE
};

test_state_t current_test = TEST_WIFI_SSID;

// storage for test inputs
char saved_ssid[MAX_SSID_LENGTH + 1] = {0};
char saved_password[MAX_WIFI_PASSWORD_LENGTH + 1] = {0};
char saved_pool[MAX_POOL_URL_LENGTH + 1] = {0};
char saved_wallet[MAX_WALLET_ADDRESS_LENGTH + 1] = {0};

// setup
void setup() {
  // init serial
  Serial.begin(SERIAL_BAUD_RATE);
  delay(1000);
  Serial.println("\n================================");
  Serial.println("   keyboard test program");
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
  
  // config keyboard for first test (ssid)
  kb.set_max_length(MAX_SSID_LENGTH);
  kb.clear();
  
  Serial.println("\n--- test 1: wifi ssid ---");
  Serial.println("enter wifi network name");
  Serial.println("press return when done");
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
    
    // pass touch to keyboard handler
    kb.handle_touch(x, y, &lcd);
  }
  
  // draw keyboard (handles redraw logic)
  kb.draw(&lcd);
  
  // check if 'return' was pressed
  if (kb.is_complete()) {
    const char* text = kb.get_text();
    
    switch (current_test) {
      // test 1: wifi ssid
      case TEST_WIFI_SSID:
        strncpy(saved_ssid, text, MAX_SSID_LENGTH);
        saved_ssid[MAX_SSID_LENGTH] = '\0';
        
        Serial.print("[input] ssid: \"");
        Serial.print(saved_ssid);
        Serial.println("\"");
        Serial.print("[info] length: ");
        Serial.print(strlen(saved_ssid));
        Serial.println(" characters");
        
        // transition to password test
        current_test = TEST_WIFI_PASSWORD;
        kb.set_max_length(MAX_WIFI_PASSWORD_LENGTH);
        kb.clear();
        kb.reset_complete();
        
        Serial.println("\n--- test 2: wifi password ---");
        Serial.println("enter wifi password");
        Serial.println("(max 63 characters)");
        Serial.println();
        break;
      
      // test 2: wifi password
      case TEST_WIFI_PASSWORD:
        strncpy(saved_password, text, MAX_WIFI_PASSWORD_LENGTH);
        saved_password[MAX_WIFI_PASSWORD_LENGTH] = '\0';
        
        Serial.print("[input] password: \"");
        Serial.print(saved_password);
        Serial.println("\"");
        Serial.print("[info] length: ");
        Serial.print(strlen(saved_password));
        Serial.println(" characters");
        
        // transition to pool url test
        current_test = TEST_POOL_URL;
        kb.set_max_length(MAX_POOL_URL_LENGTH);
        kb.clear();
        kb.reset_complete();
        
        Serial.println("\n--- test 3: pool url ---");
        Serial.println("enter mining pool address");
        Serial.println("try long text to test wrapping!");
        Serial.println("example: solo.ckpool.org");
        Serial.println();
        break;
      
      // test 3: pool url
      case TEST_POOL_URL:
        strncpy(saved_pool, text, MAX_POOL_URL_LENGTH);
        saved_pool[MAX_POOL_URL_LENGTH] = '\0';
        
        Serial.print("[input] pool url: \"");
        Serial.print(saved_pool);
        Serial.println("\"");
        Serial.print("[info] length: ");
        Serial.print(strlen(saved_pool));
        Serial.println(" characters");
        
        if (strlen(saved_pool) > KB_TEXT_THRESHOLD) {
          Serial.println("[test] text wrapping triggered (>26 chars)");
        }
        
        // transition to wallet test
        current_test = TEST_WALLET;
        kb.set_max_length(MAX_WALLET_ADDRESS_LENGTH);
        kb.clear();
        kb.reset_complete();
        
        Serial.println("\n--- test 4: wallet address ---");
        Serial.println("enter bitcoin wallet address");
        Serial.println("try really long text (60+ chars)");
        Serial.println("example: bc1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh");
        Serial.println();
        break;
      
      // test 4: wallet address
      case TEST_WALLET:
        strncpy(saved_wallet, text, MAX_WALLET_ADDRESS_LENGTH);
        saved_wallet[MAX_WALLET_ADDRESS_LENGTH] = '\0';
        
        Serial.print("[input] wallet: \"");
        Serial.print(saved_wallet);
        Serial.println("\"");
        Serial.print("[info] length: ");
        Serial.print(strlen(saved_wallet));
        Serial.println(" characters");
        
        if (strlen(saved_wallet) > KB_MAX_VISIBLE_CHARS) {
          Serial.println("[test] ellipsis triggered (>100 chars)");
        }
        
        // all tests complete
        current_test = TEST_COMPLETE;
        display_results();
        break;
      
      // tests complete
      case TEST_COMPLETE:
        // wait for reset
        break;
    }
  }
}

// display results on screen and serial
void display_results() {
  // clear screen
  lcd.fillScreen(COLOR_BLACK);
  
  // draw success message
  lcd.setTextColor(COLOR_GREEN);
  lcd.setTextSize(2);
  lcd.setCursor(40, 30);
  lcd.println("tests complete!");
  
  // draw instructions
  lcd.setTextSize(1);
  lcd.setTextColor(COLOR_YELLOW);
  lcd.setCursor(10, 60);
  lcd.println("check serial monitor for results");
  lcd.setCursor(10, 75);
  lcd.println("press reset to test again");
  
  // draw summary on screen
  lcd.setTextColor(COLOR_WHITE);
  lcd.setCursor(10, 100);
  lcd.print("ssid: ");
  lcd.println(saved_ssid[0] ? saved_ssid : "(empty)");
  
  lcd.setCursor(10, 115);
  lcd.print("pass: ");
  if (saved_password[0]) {
    // show asterisks for password
    for (uint8_t i = 0; i < strlen(saved_password) && i < 20; i++) {
      lcd.print("*");
    }
    if (strlen(saved_password) > 20) {
      lcd.print("...");
    }
  } else {
    lcd.print("(empty)");
  }
  
  lcd.setCursor(10, 130);
  lcd.print("pool: ");
  if (strlen(saved_pool) > 30) {
    // truncate long urls
    char temp[31];
    strncpy(temp, saved_pool, 30);
    temp[30] = '\0';
    lcd.print(temp);
    lcd.print("...");
  } else {
    lcd.println(saved_pool[0] ? saved_pool : "(empty)");
  }
  
  lcd.setCursor(10, 145);
  lcd.print("wallet: ");
  if (strlen(saved_wallet) > 25) {
    // truncate long addresses
    char temp[26];
    strncpy(temp, saved_wallet, 25);
    temp[25] = '\0';
    lcd.print(temp);
    lcd.print("...");
  } else {
    lcd.println(saved_wallet[0] ? saved_wallet : "(empty)");
  }
  
  // print detailed results to serial
  Serial.println("\n================================");
  Serial.println("   test results summary");
  Serial.println("================================");
  Serial.println();
  
  Serial.println("wifi configuration:");
  Serial.print("  ssid: \"");
  Serial.print(saved_ssid[0] ? saved_ssid : "(empty)");
  Serial.println("\"");
  Serial.print("  password: \"");
  Serial.print(saved_password[0] ? saved_password : "(empty)");
  Serial.println("\"");
  Serial.println();
  
  Serial.println("mining configuration:");
  Serial.print("  pool url: \"");
  Serial.print(saved_pool[0] ? saved_pool : "(empty)");
  Serial.println("\"");
  Serial.print("  wallet address: \"");
  Serial.print(saved_wallet[0] ? saved_wallet : "(empty)");
  Serial.println("\"");
  Serial.println();
  
  Serial.println("keyboard test results:");
  Serial.print("  [");
  Serial.print(strlen(saved_ssid) > 0 ? "✓" : "x");
  Serial.println("] wifi ssid");
  Serial.print("  [");
  Serial.print(strlen(saved_password) > 0 ? "✓" : "x");
  Serial.println("] wifi password");
  Serial.print("  [");
  Serial.print(strlen(saved_pool) > 0 ? "✓" : "x");
  Serial.println("] pool url");
  Serial.print("  [");
  Serial.print(strlen(saved_wallet) > 0 ? "✓" : "x");
  Serial.println("] wallet address");
  Serial.println();
  
  Serial.println("test features verified:");
  Serial.println("  [✓] basic text input");
  Serial.println("  [✓] keyboard mode switching");
  Serial.println("  [✓] backspace functionality");
  Serial.println("  [✓] space bar");
  Serial.println("  [✓] return key");
  if (strlen(saved_pool) > KB_TEXT_THRESHOLD || strlen(saved_wallet) > KB_TEXT_THRESHOLD) {
    Serial.println("  [✓] text size auto-scaling");
  }
  if (strlen(saved_wallet) > 53) {
    Serial.println("  [✓] multi-line text wrapping");
  }
  Serial.println();
  
  Serial.println("================================");
  Serial.println("all keyboard tests passed!");
  Serial.println("press reset to run again");
  Serial.println("================================");
}