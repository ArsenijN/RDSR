#pragma once
#include <Arduino.h>
#include <SD.h>
#include "config.h"

// Action codes
enum MenuAction : uint8_t {
    ACTION_NONE          = 0,
    ACTION_SD_CARD       = 1,
    ACTION_POWER         = 2,
    ACTION_BLUETOOTH     = 3,
    ACTION_CALIBRATIONS  = 4,
    ACTION_FACTORY_RESET = 5,
    ACTION_LANGUAGE      = 6,
    ACTION_TURN_OFF      = 7,
};

// ─── Thin runtime item: only what we need in RAM at once ──────────────────
struct MenuItemSlim {
    char    label[20];   // 20 bytes
    uint8_t action;      // 1 byte
    // bitmap: drawn on-demand from SD, not buffered
    char    bmpFile[13]; // "12345678.xbm\0" — 8.3 name, 13 bytes
    uint8_t bmpW;
    uint8_t bmpH;
};  // 36 bytes total — one item at a time

// ─── SDMenu: streams from SD, keeps only counts + file position ───────────
class SDMenu {
public:
    uint8_t count = 0;
    bool    sdOK  = false;

    bool begin() {
        sdOK = SD.begin(PIN_SD_CS);
        if (!sdOK) return false;
        // Count items so we know scroll bounds
        count = countItems();
        return true;
    }

    // Load one item by index into dst. Returns false if SD unavailable.
    bool getItem(uint8_t idx, MenuItemSlim& dst) const {
        if (!sdOK) { loadDefault(idx, dst); return true; }
        File f = SD.open(SD_MENU_FILE, FILE_READ);
        if (!f) { loadDefault(idx, dst); return true; }

        memset(&dst, 0, sizeof(dst));
        char line[SD_LINE_BUF];
        int8_t cur = -1;
        bool found = false;

        while (f.available()) {
            readLine(f, line);
            if (line[0] == '#' || line[0] == '\0') continue;
            if (strncmp_P(line, PSTR("[item]"), 6) == 0) {
                cur++;
                if (cur == (int8_t)idx) found = true;
                else if (cur > (int8_t)idx) break;
                continue;
            }
            if (!found) continue;
            char* eq = strchr(line, '=');
            if (!eq) continue;
            *eq = '\0';
            char* val = eq + 1;
            if      (strcmp_P(line, PSTR("label"))  == 0) strncpy(dst.label,   val, 19);
            else if (strcmp_P(line, PSTR("action")) == 0) dst.action = (uint8_t)atoi(val);
            else if (strcmp_P(line, PSTR("bitmap")) == 0) strncpy(dst.bmpFile, val, 12);
            else if (strcmp_P(line, PSTR("bmpW"))   == 0) dst.bmpW = (uint8_t)atoi(val);
            else if (strcmp_P(line, PSTR("bmpH"))   == 0) dst.bmpH = (uint8_t)atoi(val);
        }
        f.close();

        if (!found) loadDefault(idx, dst);
        return true;
    }

    // Draw bitmap for an item directly onto u8g2 at (x,y) — reads from SD,
    // uses only a one-row scratch buffer on the stack.
    // Call this inside a u8g2 page loop.
    void drawBitmap(U8G2& u8g2, const MenuItemSlim& item, uint8_t x, uint8_t y) const {
        if (!sdOK || item.bmpFile[0] == '\0' || item.bmpW == 0) return;
        char path[28];
        // "/bitmaps/" = 9 chars + name(12) + null = 22 max
        strcpy_P(path, PSTR("/bitmaps/"));
        strncat(path, item.bmpFile, 12);

        File f = SD.open(path, FILE_READ);
        if (!f) return;

        uint8_t rowBytes = (item.bmpW + 7) / 8;
        uint8_t rowBuf[4]; // max 32px wide icon; 4 bytes/row is plenty for 16px
        for (uint8_t row = 0; row < item.bmpH; row++) {
            if (f.read(rowBuf, rowBytes) != rowBytes) break;
            for (uint8_t col = 0; col < item.bmpW; col++) {
                if (rowBuf[col / 8] & (1 << (col % 8)))
                    u8g2.drawPixel(x + col, y + row);
            }
        }
        f.close();
    }

    uint8_t defaultCount() const { return 7; }

private:
    uint8_t countItems() const {
        if (!sdOK) return defaultCount();
        File f = SD.open(SD_MENU_FILE, FILE_READ);
        if (!f) return defaultCount();
        uint8_t n = 0;
        char line[SD_LINE_BUF];
        while (f.available() && n < SD_MAX_ITEMS) {
            readLine(f, line);
            if (strncmp_P(line, PSTR("[item]"), 6) == 0) n++;
        }
        f.close();
        return n ? n : defaultCount();
    }

    // Fallback when SD absent — hardcoded defaults, labels in PROGMEM
    void loadDefault(uint8_t idx, MenuItemSlim& dst) const {
        static const char l0[] PROGMEM = "SD Card";
        static const char l1[] PROGMEM = "Power features";
        static const char l2[] PROGMEM = "Bluetooth";
        static const char l3[] PROGMEM = "Calibrations";
        static const char l4[] PROGMEM = "Factory reset";
        static const char l5[] PROGMEM = "Language";
        static const char l6[] PROGMEM = "Turn off";
        static const char* const labels[] PROGMEM = {l0,l1,l2,l3,l4,l5,l6};
        static const uint8_t actions[] PROGMEM = {1,2,3,4,5,6,7};

        if (idx >= 7) return;
        strncpy_P(dst.label, (const char*)pgm_read_word(&labels[idx]), 19);
        dst.action  = pgm_read_byte(&actions[idx]);
        dst.bmpFile[0] = '\0';
        dst.bmpW = dst.bmpH = 0;
    }

    static void readLine(File& f, char* buf) {
        uint8_t i = 0;
        while (f.available() && i < SD_LINE_BUF - 1) {
            char c = f.read();
            if (c == '\r') continue;
            if (c == '\n') break;
            buf[i++] = c;
        }
        buf[i] = '\0';
    }
};
