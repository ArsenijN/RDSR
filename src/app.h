#pragma once
#include <Arduino.h>
#include <EEPROM.h>
#include "config.h"
#include "encoder.h"
#include "sd_menu.h"
#include "ui.h"

// ─── Default (fallback) settings menu – used when SD absent/fails ──
static void buildDefaultMenu(MenuData& m) {
    strncpy(m.title, "Settings", SD_MAX_LABEL - 1);
    m.count = 7;

    auto setItem = [&](uint8_t i, const char* lbl, uint8_t act) {
        strncpy(m.items[i].label, lbl, SD_MAX_LABEL - 1);
        m.items[i].action  = act;
        m.items[i].bmpData = nullptr;
        m.items[i].bmpW    = 0;
        m.items[i].bmpH    = 0;
        m.items[i].bmpFile[0] = '\0';
    };

    setItem(0, "SD Card",       ACTION_SD_CARD);
    setItem(1, "Power features",ACTION_POWER);
    setItem(2, "Bluetooth",     ACTION_BLUETOOTH);
    setItem(3, "Calibrations",  ACTION_CALIBRATIONS);
    setItem(4, "Factory reset", ACTION_FACTORY_RESET);
    setItem(5, "Language",      ACTION_LANGUAGE);
    setItem(6, "Turn off",      ACTION_TURN_OFF);
}

// ─── App ──────────────────────────────────────────────────────────
class App {
public:
    void begin(U8G2& display, Encoder& enc, SDMenuLoader& sdLoader) {
        _display   = &display;
        _enc       = &enc;
        _sdLoader  = &sdLoader;

        _ui.begin(display);

        // ── SD card ───────────────────────────────────────────────
        _sdOK = _sdLoader->begin();
        if (_sdOK) {
            _sdOK = _sdLoader->load(_settingsMenu);
        }
        if (!_sdOK) {
            buildDefaultMenu(_settingsMenu);
        }

        // ── EEPROM: first-boot detection ──────────────────────────
        uint8_t magic = EEPROM.read(EEPROM_MAGIC_ADDR);
        bool firstBoot = (magic != EEPROM_MAGIC_VALUE);

        if (firstBoot) {
            _screen = SCREEN_BOOT;
            _pendingAfterSplash = SCREEN_WELCOME;
        } else {
            uint8_t rememberScr = EEPROM.read(EEPROM_REMEMBERSCR_ADDR);
            uint8_t lastScr     = EEPROM.read(EEPROM_LAST_SCREEN_ADDR);
            _screen = SCREEN_BOOT;
            if (rememberScr && lastScr >= SCREEN_HOME) {
                _pendingAfterSplash = lastScr;
            } else {
                _pendingAfterSplash = SCREEN_HOME;
            }
        }

        _splashStart = millis();
        _selIdx = 0;
    }

    // Call every loop iteration
    void update() {
        EncoderEvent ev = _enc->poll();
        uint32_t now    = millis();

        switch (_screen) {
            // ── Boot splash ───────────────────────────────────────
            case SCREEN_BOOT:
                render();
                if (now - _splashStart >= SPLASH_DURATION_MS) {
                    goTo(_pendingAfterSplash);
                }
                break;

            // ── Welcome (first boot) ──────────────────────────────
            case SCREEN_WELCOME:
                render();
                if (ev == ENC_PRESS || ev == ENC_LONG_PRESS) {
                    // Mark first-boot done
                    EEPROM.write(EEPROM_MAGIC_ADDR, EEPROM_MAGIC_VALUE);
                    EEPROM.write(EEPROM_LAST_SCREEN_ADDR, SCREEN_HOME);
                    EEPROM.write(EEPROM_REMEMBERSCR_ADDR, 0);
                    EEPROM.write(EEPROM_LANG_ADDR, 0);
                    goTo(SCREEN_SETTINGS);
                }
                break;

            // ── Home ──────────────────────────────────────────────
            case SCREEN_HOME:
                // Encoder rotation: tune (placeholder – no Si4703 yet)
                if (ev == ENC_CW)   _homeFreqKhz += 100;
                if (ev == ENC_CCW && _homeFreqKhz >= 87600)
                    _homeFreqKhz -= 100;
                // Clamp to FM band
                if (_homeFreqKhz < 87500)  _homeFreqKhz = 87500;
                if (_homeFreqKhz > 108000) _homeFreqKhz = 108000;

                if (ev == ENC_PRESS) {
                    _selIdx = 0;
                    goTo(SCREEN_SETTINGS);
                }
                render();
                break;

            // ── Settings ──────────────────────────────────────────
            case SCREEN_SETTINGS:
                if (ev == ENC_CW) {
                    if (_selIdx < _settingsMenu.count - 1) _selIdx++;
                }
                if (ev == ENC_CCW) {
                    if (_selIdx > 0) _selIdx--;
                }
                if (ev == ENC_PRESS) {
                    handleSettingsAction(_settingsMenu.items[_selIdx].action);
                }
                if (ev == ENC_LONG_PRESS) {
                    // Long-press anywhere in settings = go back to home
                    goTo(SCREEN_HOME);
                }
                render();
                break;

            default:
                goTo(SCREEN_HOME);
                break;
        }
    }

private:
    U8G2*         _display  = nullptr;
    Encoder*      _enc      = nullptr;
    SDMenuLoader* _sdLoader = nullptr;
    UI            _ui;

    uint8_t  _screen             = SCREEN_BOOT;
    uint8_t  _pendingAfterSplash = SCREEN_HOME;
    uint32_t _splashStart        = 0;
    uint8_t  _selIdx             = 0;
    bool     _sdOK               = false;
    bool     _needRedraw         = true;

    MenuData _settingsMenu;

    // Home screen state (placeholders until Si4703 is wired up)
    uint32_t _homeFreqKhz = 98500;

    void goTo(uint8_t screen) {
        _screen     = screen;
        _needRedraw = true;
        // Persist last screen (skip boot/welcome)
        if (screen >= SCREEN_HOME) {
            EEPROM.write(EEPROM_LAST_SCREEN_ADDR, screen);
        }
    }

    void render() {
        _display->firstPage();
        do {
            switch (_screen) {
                case SCREEN_BOOT:
                    _ui.drawBoot();
                    break;
                case SCREEN_WELCOME:
                    _ui.drawWelcome();
                    break;
                case SCREEN_HOME:
                    _ui.drawHome(_homeFreqKhz, nullptr, 0, _sdOK);
                    break;
                case SCREEN_SETTINGS:
                    _ui.drawSettings(_settingsMenu, _selIdx);
                    break;
            }
        } while (_display->nextPage());
    }

    void handleSettingsAction(uint8_t action) {
        switch (action) {
            case ACTION_TURN_OFF:
                // On AVR there's no true power-off without extra hardware.
                // Put display to sleep and spin forever as a soft-off.
                _display->setPowerSave(1);
                while (true) { /* sleep */ }
                break;

            case ACTION_FACTORY_RESET:
                // Wipe magic byte → next boot is treated as first boot
                EEPROM.write(EEPROM_MAGIC_ADDR, 0xFF);
                EEPROM.write(EEPROM_LAST_SCREEN_ADDR, 0xFF);
                EEPROM.write(EEPROM_REMEMBERSCR_ADDR, 0xFF);
                EEPROM.write(EEPROM_LANG_ADDR, 0xFF);
                // Show brief confirmation then reboot via watchdog
                _display->firstPage();
                do {
                    _display->setFont(u8g2_font_4x6_tr);
                    _display->drawStr(20, 32, "Factory reset done.");
                    _display->drawStr(20, 42, "Restarting...");
                } while (_display->nextPage());
                delay(1500);
                // Soft-reset: jump to address 0 (not ideal but works on AVR)
                asm volatile ("  jmp 0");
                break;

            case ACTION_LANGUAGE:
                // Toggle EN/UA and persist
                {
                    uint8_t lang = EEPROM.read(EEPROM_LANG_ADDR);
                    lang = (lang == 0) ? 1 : 0;
                    EEPROM.write(EEPROM_LANG_ADDR, lang);
                    // TODO: propagate language to UI strings
                }
                break;

            // Stubs for future expansion
            case ACTION_SD_CARD:
            case ACTION_POWER:
            case ACTION_BLUETOOTH:
            case ACTION_CALIBRATIONS:
            default:
                // Placeholder: show a "not implemented" flash
                _display->firstPage();
                do {
                    _display->setFont(u8g2_font_4x6_tr);
                    _display->drawStr(20, 32, "Coming soon...");
                } while (_display->nextPage());
                delay(800);
                break;
        }
    }
};
