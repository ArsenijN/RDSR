#pragma once
#include <Arduino.h>
#include "config.h"

enum EncoderEvent : uint8_t {
    ENC_NONE = 0,
    ENC_CW,
    ENC_CCW,
    ENC_PRESS,
    ENC_LONG_PRESS
};

class Encoder {
public:
    void begin() {
        pinMode(PIN_ENC_CLK, INPUT_PULLUP);
        pinMode(PIN_ENC_DT,  INPUT_PULLUP);
        pinMode(PIN_ENC_SW,  INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(PIN_ENC_CLK), isrCLK, CHANGE);
        attachInterrupt(digitalPinToInterrupt(PIN_ENC_DT),  isrDT,  CHANGE);
        _lastClk  = digitalRead(PIN_ENC_CLK);
        _lastDt   = digitalRead(PIN_ENC_DT);
        _btnState = HIGH;
        _longFired = false;
        _delta = 0;
    }

    EncoderEvent poll() {
        int8_t d = 0;
        noInterrupts(); d = _delta; _delta = 0; interrupts();
        if (d > 0) return ENC_CW;
        if (d < 0) return ENC_CCW;

        bool pressed = (digitalRead(PIN_ENC_SW) == LOW);
        uint32_t now = millis();

        if (pressed && _btnState == HIGH) {
            _btnState = LOW; _btnPressTime = now; _longFired = false;
        } else if (!pressed && _btnState == LOW) {
            _btnState = HIGH;
            if (!_longFired && (now - _btnPressTime) >= BTN_DEBOUNCE_MS)
                return ENC_PRESS;
        } else if (pressed && _btnState == LOW && !_longFired) {
            if ((now - _btnPressTime) >= BTN_LONG_MS) {
                _longFired = true; return ENC_LONG_PRESS;
            }
        }
        return ENC_NONE;
    }

private:
    static volatile int8_t  _delta;
    static volatile uint8_t _lastClk;
    static volatile uint8_t _lastDt;
    uint8_t  _btnState;
    uint32_t _btnPressTime;
    bool     _longFired;

    static void isrCLK() {
        uint8_t clk = digitalRead(PIN_ENC_CLK);
        uint8_t dt  = digitalRead(PIN_ENC_DT);
        if (clk != _lastClk) { _lastClk = clk; _delta += (clk == dt) ? -1 : 1; }
    }
    static void isrDT() {
        uint8_t clk = digitalRead(PIN_ENC_CLK);
        uint8_t dt  = digitalRead(PIN_ENC_DT);
        if (dt != _lastDt) { _lastDt = dt; _delta += (clk == dt) ? 1 : -1; }
    }
};

volatile int8_t  Encoder::_delta   = 0;
volatile uint8_t Encoder::_lastClk = 0;
volatile uint8_t Encoder::_lastDt  = 0;
