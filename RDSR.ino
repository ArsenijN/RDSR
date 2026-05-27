/*
 * RDSR — Portable RDS-enabled FM Radio
 * Target: Arduino Nano V3 (ATmega328P)
 *
 * ── Pin map ────────────────────────────────────────────────────────
 *  A4/A5       I²C  — SSD1306 OLED + Si4703 (shared bus)
 *  D2          ENC_CLK  (INT0)
 *  D3          ENC_DT   (INT1)
 *  D4          ENC_SW   (button)
 *  D8          SD CS
 *  D9          Si4703 RESET
 *  D10–D13     SPI  — MicroSD (D10 is hardware SS, kept as output)
 *
 * ── Libraries needed (install via Library Manager) ─────────────────
 *  U8g2        by olikraus
 *  SD          built-in (Arduino)
 *  SPI         built-in (Arduino)
 *  EEPROM      built-in (Arduino)
 *
 * ── MVP scope ──────────────────────────────────────────────────────
 *  ✓ Boot splash  ✓ First-boot welcome  ✓ Settings menu
 *  ✓ Home screen (UI only, no Si4703 comms yet)
 *  ✓ SD card menu loading (labels + XBM bitmaps)
 *  ✓ Encoder navigation  ✓ EEPROM persistence
 *  ✗ Si4703 FM / RDS  — next milestone
 */

#include <SPI.h>
#include <Wire.h>
#include <U8g2lib.h>
#include "src/config.h"
#include "src/encoder.h"
#include "src/sd_menu.h"
#include "src/app.h"

// ─── U8g2 instance ────────────────────────────────────────────────
// SSD1306 128×64 I²C, hardware I²C, full frame buffer.
// If your display is blank, try U8G2_SSD1306_128X64_NONAME_1_HW_I2C
// (the _1_ suffix uses page-buffer mode; smaller RAM, same API).
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

Encoder      encoder;
SDMenuLoader sdMenu;
App          app;

void setup() {
    // D10 must be OUTPUT on Nano for SPI master mode, even if unused as CS
    pinMode(10, OUTPUT);

    Wire.begin();
    u8g2.begin();
    u8g2.setContrast(200);   // adjust 0-255 to taste

    encoder.begin();
    app.begin(u8g2, encoder, sdMenu);
}

void loop() {
    app.update();
}
