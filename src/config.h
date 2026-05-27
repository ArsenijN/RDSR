#pragma once

// ─── Pin assignments ───────────────────────────────────────────────────────
#define PIN_ENC_CLK   2
#define PIN_ENC_DT    3
#define PIN_ENC_SW    4
#define PIN_SI_RESET  9
#define PIN_SD_CS     8

// ─── EEPROM layout ────────────────────────────────────────────────────────
#define EEPROM_MAGIC_ADDR       0x00
#define EEPROM_LAST_SCREEN_ADDR 0x01
#define EEPROM_LANG_ADDR        0x02
#define EEPROM_REMEMBERSCR_ADDR 0x03
#define EEPROM_MAGIC_VALUE      0xAD

// ─── Display ──────────────────────────────────────────────────────────────
#define SCREEN_W  128
#define SCREEN_H  64
#define OLED_ADDR 0x3C

// ─── SD card ──────────────────────────────────────────────────────────────
#define SD_MENU_FILE  "/menu.cfg"
// Max chars we ever read into a temporary line buffer (stack, not heap)
#define SD_LINE_BUF   48
// Max menu items we will ever scroll through
#define SD_MAX_ITEMS  8

// ─── Encoder ──────────────────────────────────────────────────────────────
#define BTN_DEBOUNCE_MS  30
#define BTN_LONG_MS     600

// ─── Splash ───────────────────────────────────────────────────────────────
#define SPLASH_DURATION_MS 2200

// ─── Screen indices ───────────────────────────────────────────────────────
#define SCREEN_BOOT     0
#define SCREEN_WELCOME  1
#define SCREEN_HOME     2
#define SCREEN_SETTINGS 3
