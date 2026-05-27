/*
 * RDSR — Portable RDS-enabled FM Radio  (MVP build)
 * Target: Arduino Nano V3 (ATmega328P, 32KB flash, 2KB RAM)
 *
 * ── Pin map ──────────────────────────────────────────────────────────────
 *  A4/A5      I²C  SSD1306 OLED + Si4703 (shared bus)
 *  D2         ENC CLK (INT0)
 *  D3         ENC DT  (INT1)
 *  D4         ENC SW  (button, active-low, internal pull-up)
 *  D8         SD CS
 *  D9         Si4703 RESET
 *  D10        must be OUTPUT (SPI master SS)
 *  D11-D13    SPI  MicroSD
 *
 * ── Libraries (Library Manager) ─────────────────────────────────────────
 *  U8g2  by olikraus
 *  SD    (built-in)
 */

#include <SPI.h>
#include <Wire.h>
#include <U8g2lib.h>
#include "src/config.h"
#include "src/encoder.h"
#include "src/sd_menu.h"
#include "src/app.h"

// Page-buffer mode (_1_): 128 bytes RAM vs 1024 for full-buffer (_F_).
// The page loop in render() handles the rest.
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

Encoder encoder;
SDMenu  sdMenu;
App     app;

void setup() {
    pinMode(10, OUTPUT);  // SPI SS must be output even if unused as CS
    Wire.begin();
    u8g2.begin();
    encoder.begin();
    app.begin(u8g2, encoder, sdMenu);
}

void loop() {
    app.update();
}
