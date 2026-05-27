#pragma once
#include <Arduino.h>
#include <U8g2lib.h>
#include "config.h"
#include "sd_menu.h"

// Boot splash bitmap (16x15) — in PROGMEM, costs 0 RAM
static const uint8_t image_music_radio_bits[] PROGMEM = {
    0x00,0x18,0x00,0x06,0x80,0x01,0x60,0x00,
    0x18,0x00,0x06,0x00,0xfe,0x7f,0x39,0x80,
    0x55,0xbe,0x83,0xa2,0xd7,0xbe,0x83,0x80,
    0x55,0xaa,0x39,0x80,0xfe,0x7f
};

class UI {
public:
    void begin(U8G2& display) { _u8 = &display; }

    // ── Boot splash ───────────────────────────────────────────────────────
    void drawBoot() {
        _u8->setFontMode(1);
        _u8->setBitmapMode(1);
        _u8->setFont(u8g2_font_4x6_tr);
        _u8->drawStr(8,  13, "RDSR portable");
        _u8->drawStr(8,  19, "(RDS-enabled Radio)");
        _u8->drawStr(76, 56, "@arsenius.gen");
        _u8->drawStr(20, 63, "github.io/RDSR");
        _u8->drawXBMP(56, 25, 16, 15, image_music_radio_bits);
    }

    // ── First-boot welcome ────────────────────────────────────────────────
    void drawWelcome() {
        _u8->setFontMode(1);
        _u8->drawFrame(0, 0, 128, 64);
        _u8->drawLine(0, 8, 126, 8);
        _u8->setFont(u8g2_font_4x6_tr);
        _u8->drawStr(48,  7, "WELCOME!");
        _u8->drawStr(12, 15, "Since this is a first boot");
        _u8->drawStr(20, 21, "let's do the settings!");
        _u8->drawStr(20, 33, "(Press on the encoder)");
        _u8->drawStr(56, 61, "[OK]");
        _u8->setDrawColor(2);
        _u8->drawBox(56, 55, 15, 7);
        _u8->setDrawColor(1);
    }

    // ── Settings ──────────────────────────────────────────────────────────
    // Draws one page; call inside u8g2 page loop.
    // Reads items on-the-fly from SDMenu to avoid buffering them all.
    void drawSettings(SDMenu& menu, uint8_t sel) {
        _u8->setFontMode(1);
        _u8->setBitmapMode(1);
        _u8->drawFrame(0, 0, 128, 64);
        _u8->drawLine(0, 8, 127, 8);
        _u8->drawLine(8, 0, 8,  63);
        // Back arrow
        _u8->drawLine(2, 4, 4, 2);
        _u8->drawLine(2, 4, 6, 4);
        _u8->drawLine(2, 4, 4, 6);
        _u8->setFont(u8g2_font_4x6_tr);
        _u8->drawStr(10, 7, "Settings");

        MenuItemSlim item;
        uint8_t count = menu.count;
        for (uint8_t i = 0; i < count; i++) {
            uint8_t rowTop = 9 + i * 8;   // top of this row
            uint8_t textY  = rowTop + 6;  // text baseline

            if (i > 0) _u8->drawLine(9, rowTop, 126, rowTop);

            if (i == sel) {
                _u8->setDrawColor(2);
                _u8->drawBox(9, rowTop + 1, 118, 7);
                _u8->setDrawColor(1);
            }

            menu.getItem(i, item);  // reads from SD or returns default

            uint8_t textX = 10;
            if (item.bmpFile[0]) {
                menu.drawBitmap(*_u8, item, 10, rowTop + 1);
                textX = 10 + item.bmpW + 2;
            }
            _u8->drawStr(textX, textY, item.label);
        }

        // Cursor nub on left rail
        _u8->drawBox(2, 10 + sel * 8, 5, 5);
    }

    // ── Home screen ───────────────────────────────────────────────────────
    // freq_khz: 87500–108000. rssi: 0–75.
    void drawHome(uint32_t freq_khz, const char* rdsText, uint8_t rssi, bool sdOK) {
        _u8->setFontMode(1);
        _u8->drawFrame(0, 0, 128, 64);
        _u8->drawLine(0, 8, 127, 8);
        _u8->setFont(u8g2_font_4x6_tr);
        _u8->drawStr(2, 7, "RDSR");
        if (sdOK) _u8->drawStr(112, 7, "SD");

        // Frequency — bigger font, centred
        char buf[10];
        uint16_t mhz  = (uint16_t)(freq_khz / 1000);
        uint8_t  frac = (uint8_t)((freq_khz % 1000) / 100);
        // manual itoa to avoid snprintf bloat
        uint8_t pos = 0;
        if (mhz >= 100) buf[pos++] = '0' + mhz / 100;
        buf[pos++] = '0' + (mhz % 100) / 10;
        buf[pos++] = '0' + mhz % 10;
        buf[pos++] = '.';
        buf[pos++] = '0' + frac;
        buf[pos++] = ' '; buf[pos++] = 'M'; buf[pos++] = 'H'; buf[pos++] = 'z';
        buf[pos]   = '\0';

        _u8->setFont(u8g2_font_7x14B_tr);
        uint8_t fw = _u8->getStrWidth(buf);
        _u8->drawStr((128 - fw) / 2, 30, buf);

        // RSSI blocks
        _u8->setFont(u8g2_font_4x6_tr);
        _u8->drawStr(2, 40, "SIG:");
        uint8_t bars = (rssi >= 60) ? 5 : (rssi >= 45) ? 4 :
                       (rssi >= 30) ? 3 : (rssi >= 15) ? 2 : (rssi >= 5) ? 1 : 0;
        for (uint8_t b = 0; b < 5; b++) {
            uint8_t bx = 22 + b * 7;
            if (b < bars) _u8->drawBox(bx, 35, 5, 5);
            else          _u8->drawFrame(bx, 35, 5, 5);
        }

        // RDS text (clipped to 18 chars)
        _u8->drawStr(2, 50, "RDS:");
        if (rdsText && rdsText[0]) {
            char clip[19];
            strncpy(clip, rdsText, 18); clip[18] = '\0';
            _u8->drawStr(22, 50, clip);
        } else {
            _u8->drawStr(22, 50, "---");
        }

        _u8->drawLine(0, 55, 127, 55);
        _u8->drawStr(2, 63, "turn=tune  press=menu");
    }

private:
    U8G2* _u8 = nullptr;
};
