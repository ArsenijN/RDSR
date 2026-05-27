#pragma once

// ─── Pin assignments ───────────────────────────────────────────────
#define PIN_ENC_CLK   2    // Encoder A (interrupt-capable)
#define PIN_ENC_DT    3    // Encoder B (interrupt-capable)
#define PIN_ENC_SW    4    // Encoder button (active-low, internal pull-up)
#define PIN_SI_RESET  9    // Si4703 RESET (active-low init sequence)
#define PIN_SD_CS     8    // MicroSD chip-select

// I²C: A4=SDA, A5=SCL  (shared: SSD1306 + Si4703)
// SPI: D11=MOSI, D12=MISO, D13=SCK, D8=CS  (MicroSD)

// ─── EEPROM layout ────────────────────────────────────────────────
// Address  Size  Description
// 0x00     1     Magic byte (0xRD = first-boot done)
// 0x01     1     Last screen index (0=home, 1=settings, …)
// 0x02     1     Settings: language (0=EN, 1=UA)
// 0x03     1     Settings: remember last screen (0/1)
#define EEPROM_MAGIC_ADDR       0x00
#define EEPROM_LAST_SCREEN_ADDR 0x01
#define EEPROM_LANG_ADDR        0x02
#define EEPROM_REMEMBERSCR_ADDR 0x03
#define EEPROM_MAGIC_VALUE      0xRD   // intentional hex – see note below

// Note: 0xRD is not valid hex. We use 0xAD as the sentinel ("RDSR sentinel").
#undef  EEPROM_MAGIC_VALUE
#define EEPROM_MAGIC_VALUE      0xAD

// ─── Display ──────────────────────────────────────────────────────
#define SCREEN_W  128
#define SCREEN_H  64
#define OLED_ADDR 0x3C   // most common; try 0x3D if blank

// ─── SD card ──────────────────────────────────────────────────────
#define SD_MENU_FILE   "/menu.cfg"
#define SD_BITMAP_DIR  "/bitmaps/"
#define SD_MAX_ITEMS   16
#define SD_MAX_LABEL   24   // chars per menu label
#define SD_MAX_BMPNAME 16   // chars for bitmap filename (without path)

// ─── Encoder debounce ─────────────────────────────────────────────
#define ENC_DEBOUNCE_MS  5
#define BTN_DEBOUNCE_MS  30
#define BTN_LONG_MS     600   // long-press threshold

// ─── Splash screen timing ─────────────────────────────────────────
#define SPLASH_DURATION_MS 2200

// ─── Screen indices ───────────────────────────────────────────────
#define SCREEN_BOOT     0
#define SCREEN_WELCOME  1
#define SCREEN_HOME     2
#define SCREEN_SETTINGS 3
