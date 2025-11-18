// config.h
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// display configuration

// screen dimensions (landscape)
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

// Byte-swapped RGB565 colors for ILI9341
#define COLOR_BLACK 0x0000
#define COLOR_WHITE 0xFFFF
#define COLOR_RED 0xF800
#define COLOR_GREEN 0x07E0
#define COLOR_BLUE 0x001F
#define COLOR_NICEBLUE 0x03DB
#define COLOR_YELLOW 0xFFE0
#define COLOR_CYAN 0x07FF
#define COLOR_MAGENTA 0xF81F
#define COLOR_ORANGE 0xFD20
#define COLOR_DARKGREEN2 0x8410
#define COLOR_LIGHTGRAY 0xBDF7
#define COLOR_DARKGREEN 0x03E0

// keyboard configuration

// keyboard layout geometry
#define KB_INPUT_AREA_HEIGHT 62
#define KB_KEYBOARD_Y_START 62
#define KB_KEYBOARD_HEIGHT 190
#define KB_KEY_SPACING 2

// key counts
#define MAX_KEYS 32  // max keys for any layout

// key dimensions
#define KB_KEY_WIDTH 30
#define KB_KEY_HEIGHT 42
#define KB_KEY_WIDE_WIDTH 42      // shift backspace
#define KB_KEY_SPACE_WIDTH 140
#define KB_KEY_SPECIAL_WIDTH 55   // mode switch return

// input area text sizing
#define KB_TEXT_SIZE_LARGE 2      // short text (<= 26 chars)
#define KB_TEXT_SIZE_SMALL 1      // long text (> 26 chars)
#define KB_TEXT_THRESHOLD 26      // char count to switch text size
#define KB_MAX_VISIBLE_CHARS 100  // max chars before ellipsis

// timing
#define KB_PRESS_HIGHLIGHT_MS 100  // key press highlight duration (ms)

// keyboard colors
#define KB_BG_COLOR COLOR_BLACK
#define KB_BORDER_COLOR COLOR_WHITE
#define KB_TEXT_COLOR COLOR_WHITE
#define KB_KEY_NORMAL_COLOR COLOR_LIGHTGRAY
#define KB_KEY_PRESSED_COLOR COLOR_DARKGREEN2
#define KB_KEY_TEXT_COLOR COLOR_BLACK

// input length limits

// wifi config limits (802.11 std)
#define MAX_SSID_LENGTH 31        // 31 chars + null
#define MAX_WIFI_PASSWORD_LENGTH 63   // 63 chars + null (wpa2)

// pool config limits
#define MAX_POOL_ADDRESS_LENGTH 127 // 127 chars + null
#define MAX_POOL_NAME_LENGTH 20     // 20 chars + null

// wallet config limits
#define MAX_WALLET_ADDRESS_LENGTH 62  // 62 chars + null (taproot)
#define MAX_WALLET_NAME_LENGTH 20     // 20 chars + null

// pin definitions (esp32-s3 zero)

// spi display pins
#define PIN_LCD_SCLK 12
#define PIN_LCD_MOSI 11
#define PIN_LCD_MISO 13
#define PIN_LCD_DC 6
#define PIN_LCD_CS 5
#define PIN_LCD_RST 4

// i2c touch pins
#define PIN_TOUCH_SDA 2
#define PIN_TOUCH_SCL 1
#define PIN_TOUCH_INT 3
#define TOUCH_I2C_ADDR 0x38       // ft6336 i2c address

// display hardware configuration

// spi configuration
#define SPI_FREQUENCY_WRITE 40000000  // 40mhz write
#define SPI_FREQUENCY_READ 16000000   // 16mhz read

// touch configuration
#define TOUCH_I2C_FREQUENCY 400000    // 400khz

// touch coordinate ranges
#define TOUCH_X_MIN 0
#define TOUCH_X_MAX 239
#define TOUCH_Y_MIN 0
#define TOUCH_Y_MAX 319

// network timing
#define WIFI_CONNECT_TIMEOUT_MS 10000     // wifi connect timeout (ms)

//ui

// header area
#define HEADER_HEIGHT 39
#define HEADER_TEXT_X 40
#define HEADER_TEXT_Y 13
#define HEADER_NETWORK_Y 10

// back button at top
#define BACK_BUTTON_X 2
#define BACK_BUTTON_Y 3
#define BACK_BUTTON_TEXT_X 5
#define BACK_BUTTON_TEXT_Y 11
#define BACK_BUTTON_HEIGHT 30
#define BACK_BUTTON_W 30

// network list items
#define LIST_START_X 5
#define LIST_START_Y 40
#define ITEM_HEIGHT 40
#define ITEMS_PER_PAGE 4
#define ITEM_GAP 2

//pop-up menu
#define POPUP_MENU_X 30
#define POPUP_MENU_Y 90
#define POPUP_MENU_HEIGHT 60
#define POPUP_MENU_WIDTH 260

#define POPUP_MENU_BUTTON_X (POPUP_MENU_X + 10)
#define POPUP_MENU_BUTTON_Y (POPUP_MENU_Y + 15)
#define POPUP_MENU_BUTTON_HEIGHT 30
#define POPUP_MENU_BUTTON_WIDTH 50
#define POPUP_MENU_BUTTON_GAP 10
#define POPUP_MENU_BUTTON_TEXT_X (POPUP_MENU_BUTTON_X + 5)
#define POPUP_MENU_BUTTON_TEXT_Y (POPUP_MENU_BUTTON_Y + 10)


#endif