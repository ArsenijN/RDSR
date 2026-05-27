#pragma once
#include <Arduino.h>
#include <U8g2lib.h>
#include "config.h"
#include "sd_menu.h"

// ─── Boot splash bitmap (from your Lopaka draft) ──────────────────
static const unsigned char image_music_radio_bits[] PROGMEM = {
    0x00,0x18,0x00,0x06,0x80,0x01,0x60,0x00,
    0x18,0x00,0x06,0x00,0xfe,0x7f,0x39,0x80,
    0x55,0xbe,0x83,0xa2,0xd7,0xbe,0x83,0x80,
    0x55,0xaa,0x39,0x80,0xfe,0x7f
};

// ─── Renderer ─────────────────────────────────────────────────────
class UI {
public:
    void begin(U8G2& display) {
        _u8 = &display;
    }

    // ── Boot / splash ─────────────────────────────────────────────
    void drawBoot() {
        _u8->setFontMode(1);
        _u8->setBitmapMode(1);
        _u8->setFont(u8g2_font_4x6_tr);
        _u8->drawStr(8, 13, "RDSR portable");
        _u8->drawStr(8, 19, "(Radio Data System-enabled");
        _u8->drawStr(8, 25, "Radio)");
        _u8->drawStr(76, 56, "@arsenius.gen");
        _u8->drawStr(36, 63, "arsenijn.github.io/RDSR");
        _u8->drawXBMP(56, 25, 16, 15, image_music_radio_bits);
    }

    // ── First-boot welcome ────────────────────────────────────────
    void drawWelcome() {
        _u8->setFontMode(1);
        _u8->setBitmapMode(1);
        _u8->drawFrame(0, 0, 128, 64);
        _u8->drawLine(0, 8, 126, 8);
        _u8->setFont(u8g2_font_4x6_tr);
        _u8->drawStr(48, 7, "WELCOME!");
        _u8->drawStr(12, 15, "Since this is a first boot");
        _u8->drawStr(20, 21, "let's do the settings!");
        _u8->drawStr(20, 33, "(Press on the encoder)");
        _u8->drawStr(56, 61, "[OK]");
        _u8->setDrawColor(2);
        _u8->drawBox(56, 55, 15, 7);
        _u8->setDrawColor(1);
    }

    // ── Settings screen ───────────────────────────────────────────
    // sel = currently highlighted item index (0 = "SD Card", …, 6 = "Turn off")
    void drawSettings(const MenuData& menu, uint8_t sel) {
        _u8->setFontMode(1);
        _u8->setBitmapMode(1);

        _u8->drawFrame(0, 0, 128, 64);
        _u8->drawLine(0, 8, 127, 8);
        _u8->drawLine(8, 0, 8, 63);

        // Back arrow at top-left (layers 4-6 from Lopaka)
        _u8->drawLine(2, 4, 4, 2);
        _u8->drawLine(2, 4, 6, 4);
        _u8->drawLine(2, 4, 4, 6);

        // Title
        _u8->setFont(u8g2_font_4x6_tr);
        _u8->drawStr(10, 7, menu.title[0] ? menu.title : "Settings");

        // Draw items. Each row is 8px tall, first item y=15.
        // Separator lines match the Lopaka draft spacing.
        uint8_t count = menu.count;
        for (uint8_t i = 0; i < count; i++) {
            uint8_t y = 9 + (i + 1) * 8;   // 17, 25, 33, …
            uint8_t textY = y - 1;           // baseline

            // Separator above item (except first — header line handles that)
            if (i > 0) _u8->drawLine(9, y - 8, 126, y - 8);

            // Highlight selected row (XOR box)
            if (i == sel) {
                _u8->setDrawColor(2);
                _u8->drawBox(9, y - 7, 118, 7);
                _u8->setDrawColor(1);
            }

            // Bitmap icon (if loaded)
            uint8_t textX = 10;
            if (menu.items[i].bmpData) {
                _u8->drawXBMP(10, y - 7, menu.items[i].bmpW, menu.items[i].bmpH,
                              menu.items[i].bmpData);
                textX = 10 + menu.items[i].bmpW + 2;
            }

            _u8->drawStr(textX, textY, menu.items[i].label);
        }

        // Cursor box on left rail (matches Lopaka layer 22)
        uint8_t boxY = 10 + sel * 8;
        _u8->drawBox(2, boxY, 5, 5);
    }

    // ── Home screen (MVP placeholder – expand when radio is wired) ──
    // freq_khz: e.g. 98500 for 98.5 MHz. Pass 0 when radio not active.
    // rdsText:  RDS RadioText string or nullptr.
    // rssi:     0-75 (from Si4703 register). Pass 0 when not active.
    void drawHome(uint32_t freq_khz, const char* rdsText,
                  uint8_t rssi, bool sdOK) {
        _u8->setFontMode(1);
        _u8->setBitmapMode(1);

        _u8->drawFrame(0, 0, 128, 64);

        // ── Header bar ────────────────────────────────────────────
        _u8->drawLine(0, 8, 127, 8);
        _u8->setFont(u8g2_font_4x6_tr);
        _u8->drawStr(2, 7, "RDSR");

        // SD indicator (top-right)
        if (sdOK) _u8->drawStr(112, 7, "SD");

        // ── Frequency ─────────────────────────────────────────────
        char freqBuf[12];
        if (freq_khz > 0) {
            uint16_t mhz  = freq_khz / 1000;
            uint16_t frac = (freq_khz % 1000) / 100;
            snprintf(freqBuf, sizeof(freqBuf), "%u.%u MHz", mhz, frac);
        } else {
            strncpy(freqBuf, "-- MHz", sizeof(freqBuf));
        }
        _u8->setFont(u8g2_font_7x14B_tr);
        uint8_t fw = _u8->getStrWidth(freqBuf);
        _u8->drawStr((128 - fw) / 2, 30, freqBuf);  // centered

        // ── RSSI bar (5 blocks) ───────────────────────────────────
        _u8->setFont(u8g2_font_4x6_tr);
        _u8->drawStr(2, 40, "SIG:");
        uint8_t bars = (rssi >= 60) ? 5 :
                       (rssi >= 45) ? 4 :
                       (rssi >= 30) ? 3 :
                       (rssi >= 15) ? 2 :
                       (rssi >=  5) ? 1 : 0;
        for (uint8_t b = 0; b < 5; b++) {
            uint8_t bx = 22 + b * 7;
            if (b < bars) _u8->drawBox(bx, 35, 5, 5);
            else          _u8->drawFrame(bx, 35, 5, 5);
        }

        // ── RDS RadioText ─────────────────────────────────────────
        if (rdsText && rdsText[0]) {
            _u8->drawStr(2, 50, "RDS:");
            // Clip to 18 chars to fit screen width
            char clip[20];
            strncpy(clip, rdsText, 18);
            clip[18] = '\0';
            _u8->drawStr(22, 50, clip);
        } else {
            _u8->drawStr(2, 50, "RDS: ---");
        }

        // ── Footer hint ───────────────────────────────────────────
        _u8->drawLine(0, 55, 127, 55);
        _u8->drawStr(2, 63, "turn=tune  press=menu");
    }

private:
    U8G2* _u8 = nullptr;
};
