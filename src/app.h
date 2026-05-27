#pragma once
#include <Arduino.h>
#include <EEPROM.h>
#include <U8g2lib.h>
#include "config.h"
#include "encoder.h"
#include "sd_menu.h"
#include "ui.h"

class App {
public:
    void begin(U8G2& display, Encoder& enc, SDMenu& sd) {
        _display = &display;
        _enc     = &enc;
        _sd      = &sd;
        _ui.begin(display);

        _sdOK = _sd->begin();

        uint8_t magic = EEPROM.read(EEPROM_MAGIC_ADDR);
        bool firstBoot = (magic != EEPROM_MAGIC_VALUE);

        if (firstBoot) {
            _pendingAfterSplash = SCREEN_WELCOME;
        } else {
            uint8_t remScr  = EEPROM.read(EEPROM_REMEMBERSCR_ADDR);
            uint8_t lastScr = EEPROM.read(EEPROM_LAST_SCREEN_ADDR);
            _pendingAfterSplash = (remScr && lastScr >= SCREEN_HOME)
                                  ? lastScr : SCREEN_HOME;
        }

        _screen      = SCREEN_BOOT;
        _splashStart = millis();
        _selIdx      = 0;
        _freqKhz     = 98500UL;
    }

    void update() {
        EncoderEvent ev = _enc->poll();
        uint32_t now    = millis();

        switch (_screen) {

            case SCREEN_BOOT:
                render();
                if (now - _splashStart >= SPLASH_DURATION_MS)
                    goTo(_pendingAfterSplash);
                break;

            case SCREEN_WELCOME:
                render();
                if (ev == ENC_PRESS || ev == ENC_LONG_PRESS) {
                    EEPROM.write(EEPROM_MAGIC_ADDR,       EEPROM_MAGIC_VALUE);
                    EEPROM.write(EEPROM_LAST_SCREEN_ADDR, SCREEN_HOME);
                    EEPROM.write(EEPROM_REMEMBERSCR_ADDR, 0);
                    EEPROM.write(EEPROM_LANG_ADDR,        0);
                    goTo(SCREEN_SETTINGS);
                }
                break;

            case SCREEN_HOME:
                if (ev == ENC_CW  && _freqKhz < 108000UL) _freqKhz += 100;
                if (ev == ENC_CCW && _freqKhz >  87500UL) _freqKhz -= 100;
                if (ev == ENC_PRESS) { _selIdx = 0; goTo(SCREEN_SETTINGS); }
                render();
                break;

            case SCREEN_SETTINGS:
                if (ev == ENC_CW  && _selIdx < _sd->count - 1) _selIdx++;
                if (ev == ENC_CCW && _selIdx > 0)               _selIdx--;
                if (ev == ENC_PRESS)      handleAction();
                if (ev == ENC_LONG_PRESS) goTo(SCREEN_HOME);
                render();
                break;

            default:
                goTo(SCREEN_HOME);
        }
    }

private:
    U8G2*    _display = nullptr;
    Encoder* _enc     = nullptr;
    SDMenu*  _sd      = nullptr;
    UI       _ui;

    uint8_t  _screen             = SCREEN_BOOT;
    uint8_t  _pendingAfterSplash = SCREEN_HOME;
    uint32_t _splashStart        = 0;
    uint8_t  _selIdx             = 0;
    bool     _sdOK               = false;
    uint32_t _freqKhz            = 98500UL;

    void goTo(uint8_t s) {
        _screen = s;
        if (s >= SCREEN_HOME)
            EEPROM.write(EEPROM_LAST_SCREEN_ADDR, s);
    }

    void render() {
        _display->firstPage();
        do {
            switch (_screen) {
                case SCREEN_BOOT:     _ui.drawBoot();                          break;
                case SCREEN_WELCOME:  _ui.drawWelcome();                       break;
                case SCREEN_HOME:     _ui.drawHome(_freqKhz,nullptr,0,_sdOK); break;
                case SCREEN_SETTINGS: _ui.drawSettings(*_sd, _selIdx);        break;
            }
        } while (_display->nextPage());
    }

    void handleAction() {
        MenuItemSlim item;
        _sd->getItem(_selIdx, item);

        switch (item.action) {

            case ACTION_TURN_OFF:
                _display->setPowerSave(1);
                while (true) {}
                break;

            case ACTION_FACTORY_RESET:
                EEPROM.write(EEPROM_MAGIC_ADDR,       0xFF);
                EEPROM.write(EEPROM_LAST_SCREEN_ADDR, 0xFF);
                EEPROM.write(EEPROM_REMEMBERSCR_ADDR, 0xFF);
                EEPROM.write(EEPROM_LANG_ADDR,        0xFF);
                _display->firstPage();
                do {
                    _display->setFont(u8g2_font_4x6_tr);
                    _display->drawStr(10, 28, "Factory reset done.");
                    _display->drawStr(10, 40, "Restarting...");
                } while (_display->nextPage());
                delay(1500);
                asm volatile ("jmp 0");
                break;

            case ACTION_LANGUAGE: {
                uint8_t lang = EEPROM.read(EEPROM_LANG_ADDR);
                EEPROM.write(EEPROM_LANG_ADDR, lang ? 0 : 1);
                break;
            }

            default:
                // Stub: flash "Coming soon"
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
