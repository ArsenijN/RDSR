#pragma once
#include <Arduino.h>
#include <SD.h>
#include "config.h"

// ─── A single menu item loaded from SD ────────────────────────────
struct MenuItem {
    char     label[SD_MAX_LABEL];
    char     bmpFile[SD_MAX_BMPNAME];  // "" = no bitmap
    uint8_t  bmpW;
    uint8_t  bmpH;
    uint8_t* bmpData;   // heap-allocated; NULL if no bitmap
    uint8_t  action;    // application-defined action code (see actions below)
};

// Action codes (match what the Settings screen items do)
enum MenuAction : uint8_t {
    ACTION_NONE         = 0,
    ACTION_SD_CARD      = 1,
    ACTION_POWER        = 2,
    ACTION_BLUETOOTH    = 3,
    ACTION_CALIBRATIONS = 4,
    ACTION_FACTORY_RESET= 5,
    ACTION_LANGUAGE     = 6,
    ACTION_TURN_OFF     = 7,
    ACTION_HOME         = 8,
    // 0x10+ reserved for user-defined SD addons
};

// ─── Menu container ───────────────────────────────────────────────
struct MenuData {
    char     title[SD_MAX_LABEL];
    MenuItem items[SD_MAX_ITEMS];
    uint8_t  count;
};

class SDMenuLoader {
public:
    bool begin() {
        return SD.begin(PIN_SD_CS);
    }

    // Loads SD_MENU_FILE into dst. Returns true on success.
    // On failure, dst is untouched and you should fall back to defaults.
    bool load(MenuData& dst) {
        if (!SD.exists(SD_MENU_FILE)) return false;

        File f = SD.open(SD_MENU_FILE, FILE_READ);
        if (!f) return false;

        dst.count = 0;
        memset(dst.title, 0, sizeof(dst.title));

        // ── Parse line by line ────────────────────────────────────
        // Format (INI-like):
        //   title=Settings
        //   [item]
        //   label=SD Card
        //   bitmap=sd_card.xbm    (optional)
        //   bmpW=16               (required if bitmap set)
        //   bmpH=16
        //   action=1
        //   [item]
        //   ...
        //
        // Lines starting with # are comments.

        MenuItem* cur = nullptr;
        char line[64];

        while (f.available() && dst.count <= SD_MAX_ITEMS) {
            uint8_t len = readLine(f, line, sizeof(line));
            if (len == 0 || line[0] == '#') continue;

            if (strncmp(line, "[item]", 6) == 0) {
                if (dst.count < SD_MAX_ITEMS) {
                    cur = &dst.items[dst.count++];
                    memset(cur, 0, sizeof(MenuItem));
                } else {
                    cur = nullptr;
                }
                continue;
            }

            char* eq = strchr(line, '=');
            if (!eq) continue;
            *eq = '\0';
            char* key = line;
            char* val = eq + 1;

            if (strcmp(key, "title") == 0) {
                strncpy(dst.title, val, SD_MAX_LABEL - 1);
            } else if (cur) {
                if      (strcmp(key, "label")  == 0) strncpy(cur->label,   val, SD_MAX_LABEL - 1);
                else if (strcmp(key, "bitmap") == 0) strncpy(cur->bmpFile, val, SD_MAX_BMPNAME - 1);
                else if (strcmp(key, "bmpW")   == 0) cur->bmpW   = (uint8_t)atoi(val);
                else if (strcmp(key, "bmpH")   == 0) cur->bmpH   = (uint8_t)atoi(val);
                else if (strcmp(key, "action") == 0) cur->action = (uint8_t)atoi(val);
            }
        }
        f.close();

        // ── Load bitmaps for items that have one ──────────────────
        for (uint8_t i = 0; i < dst.count; i++) {
            MenuItem& it = dst.items[i];
            if (it.bmpFile[0] == '\0' || it.bmpW == 0 || it.bmpH == 0) {
                it.bmpData = nullptr;
                continue;
            }
            char path[32];
            snprintf(path, sizeof(path), "%s%s", SD_BITMAP_DIR, it.bmpFile);
            it.bmpData = loadXBM(path, it.bmpW, it.bmpH);
            // NULL is safe – the renderer checks before drawing
        }

        return (dst.count > 0);
    }

    // Free heap memory allocated for bitmaps
    void free(MenuData& d) {
        for (uint8_t i = 0; i < d.count; i++) {
            if (d.items[i].bmpData) {
                ::free(d.items[i].bmpData);
                d.items[i].bmpData = nullptr;
            }
        }
    }

private:
    // Read one '\n'-terminated line into buf (null-terminated, LF stripped).
    uint8_t readLine(File& f, char* buf, uint8_t maxLen) {
        uint8_t i = 0;
        while (f.available() && i < maxLen - 1) {
            char c = f.read();
            if (c == '\r') continue;
            if (c == '\n') break;
            buf[i++] = c;
        }
        buf[i] = '\0';
        return i;
    }

    // Load a raw binary XBM data file (just the byte array, no C header).
    // Width and height come from the menu.cfg entry so we know how many bytes.
    uint8_t* loadXBM(const char* path, uint8_t w, uint8_t h) {
        uint16_t bytes = ((uint16_t)((w + 7) / 8)) * h;
        if (!SD.exists(path)) return nullptr;
        File f = SD.open(path, FILE_READ);
        if (!f) return nullptr;
        if ((uint32_t)f.size() < bytes) { f.close(); return nullptr; }

        uint8_t* buf = (uint8_t*)malloc(bytes);
        if (!buf) { f.close(); return nullptr; }
        f.read(buf, bytes);
        f.close();
        return buf;
    }
};
