#pragma once
#include <Arduino.h>
#include "config.h"

// ─── Events emitted by Encoder::poll() ────────────────────────────
enum EncoderEvent : uint8_t {
    ENC_NONE = 0,
    ENC_CW,          // clockwise rotation
    ENC_CCW,         // counter-clockwise rotation
    ENC_PRESS,       // short press (released before BTN_LONG_MS)
    ENC_LONG_PRESS   // held >= BTN_LONG_MS
};

class Encoder {
public:
    void begin() {
        pinMode(PIN_ENC_CLK, INPUT_PULLUP);
        pinMode(PIN_ENC_DT,  INPUT_PULLUP);
        pinMode(PIN_ENC_SW,  INPUT_PULLUP);

        // Attach interrupts for both edges on CLK and DT
        attachInterrupt(digitalPinToInterrupt(PIN_ENC_CLK), isrCLK, CHANGE);
        attachInterrupt(digitalPinToInterrupt(PIN_ENC_DT),  isrDT,  CHANGE);

        _lastClk      = digitalRead(PIN_ENC_CLK);
        _btnState     = HIGH;
        _btnPressTime = 0;
        _longFired    = false;
        _delta        = 0;
    }

    // Call once per loop iteration. Returns one event (or ENC_NONE).
    EncoderEvent poll() {
        // ── Rotation ──────────────────────────────────────────────
        int8_t d = 0;
        noInterrupts();
        d = _delta;
        _delta = 0;
        interrupts();

        if (d > 0) return ENC_CW;
        if (d < 0) return ENC_CCW;

        // ── Button ────────────────────────────────────────────────
        bool pressed = (digitalRead(PIN_ENC_SW) == LOW);
        uint32_t now = millis();

        if (pressed && _btnState == HIGH) {
            // just pressed
            _btnState     = LOW;
            _btnPressTime = now;
            _longFired    = false;
        } else if (!pressed && _btnState == LOW) {
            // just released
            _btnState = HIGH;
            if (!_longFired) {
                uint32_t held = now - _btnPressTime;
                if (held >= BTN_DEBOUNCE_MS) return ENC_PRESS;
            }
        } else if (pressed && _btnState == LOW && !_longFired) {
            // still held
            if ((now - _btnPressTime) >= BTN_LONG_MS) {
                _longFired = true;
                return ENC_LONG_PRESS;
            }
        }

        return ENC_NONE;
    }

private:
    // Shared between ISR and main context – volatile
    static volatile int8_t _delta;
    static volatile uint8_t _lastClk;
    static volatile uint8_t _lastDt;

    uint8_t  _btnState;
    uint32_t _btnPressTime;
    bool     _longFired;

    // Gray-code decoder using both CLK and DT edges
    static void isrCLK() {
        uint8_t clk = digitalRead(PIN_ENC_CLK);
        uint8_t dt  = digitalRead(PIN_ENC_DT);
        if (clk != _lastClk) {
            _lastClk = clk;
            _delta += (clk == dt) ? -1 : 1;
        }
    }
    static void isrDT() {
        uint8_t clk = digitalRead(PIN_ENC_CLK);
        uint8_t dt  = digitalRead(PIN_ENC_DT);
        if (dt != _lastDt) {
            _lastDt = dt;
            _delta += (clk == dt) ? 1 : -1;
        }
    }
};

volatile int8_t  Encoder::_delta   = 0;
volatile uint8_t Encoder::_lastClk = 0;
volatile uint8_t Encoder::_lastDt  = 0;
