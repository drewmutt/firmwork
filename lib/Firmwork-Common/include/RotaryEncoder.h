#pragma once
#include <Arduino.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
class RotaryEncoder: public Updateable {
public:
  using StepCb = std::function<void(RotaryEncoder*, int8_t)>;
  using ClickCb = std::function<void(RotaryEncoder*)>;

  RotaryEncoder(byte pinClk, byte pinDT, byte pinSw)
  : _clk(static_cast<gpio_num_t>(pinClk)), _dt(static_cast<gpio_num_t>(pinDT)), _sw(static_cast<gpio_num_t>(pinSw)) {}

  // tuning
  void setEdgesPerDetent(uint8_t n)  { _edgesPerDetent = (n==1||n==2||n==4)? n:4; }
  void setEdgeDebounceMs(uint8_t ms) { _edgeDebounceMs = ms; }       // 0–3 typical
  void setButtonDebounceMs(uint8_t ms){ _btnDebounceMs = ms; }       // 10–30 typical
  void setReversalSlop(uint8_t e)    { _reversalSlop = (e<=3)? e:1; }

  void begin() {
    pinMode((uint8_t)_clk, INPUT_PULLUP);
    pinMode((uint8_t)_dt,  INPUT_PULLUP);
    pinMode((uint8_t)_sw,  INPUT_PULLUP);

    _prev = (gpio_get_level(_clk) << 1) | gpio_get_level(_dt);

    // CHANGE on both lines; ISR only queues samples
    attachInterruptArg((uint8_t)_clk, isrLog, this, CHANGE);
    attachInterruptArg((uint8_t)_dt,  isrLog, this, CHANGE);
  }

  void setOnStep(StepCb onStep)
  {
    _onStep = onStep;
  }

  void setOnClick(ClickCb onClick)
  {
    _onClick = onClick;
  }

  // call from loop()
  void update() {
    // drain ring buffer without keeping interrupts off for long
    while (true) {
      uint8_t s;
      noInterrupts();
      if (_rbR == _rbW) { interrupts(); break; }
      s = _rb[_rbR]; _rbR = (_rbR + 1) & _rbMask;
      interrupts();

      // edge debounce in task context
      const uint32_t now = micros();
      if (_edgeDebounceMs && (now - _lastEdgeUs) < (uint32_t)_edgeDebounceMs * 1000UL) continue;
      _lastEdgeUs = now;

      // decode transition
      const uint8_t curr = s;
      const uint8_t seq  = ( _prev << 2 ) | curr;
      int8_t d = 0;
      // forward edges
      if (seq==0x1 || seq==0x7 || seq==0xE || seq==0x8) d = +1;
      // backward edges
      else if (seq==0x2 || seq==0xB || seq==0xD || seq==0x4) d = -1;
      _prev = curr;
      if (!d) continue;

      // accumulate edges to full detent; tolerate tiny reversals
      int8_t a = _accum + d;
      if ((a > 0 && _accum < 0) || (a < 0 && _accum > 0)) {
        if (abs(_accum) <= _reversalSlop) a = d;
      }

      const int8_t thresh = _edgesPerDetent;
      if (a >= +thresh) { _accum = 0; if (_onStep) _onStep(this, -1); }
      else if (a <= -thresh) { _accum = 0; if (_onStep) _onStep(this, +1); }
      else { _accum = a; }
    }

    // button debounce (active-low)
    const uint32_t ms = millis();
    const bool sample = (gpio_get_level(_sw) == 0);
    if (sample != _btnLastSample) { _btnLastSample = sample; _btnLastMs = ms; }
    if ((ms - _btnLastMs) >= _btnDebounceMs && sample != _btnStable) {
      _btnStable = sample;
      if (_btnStable && _onClick) _onClick(this);
    }
  }


public:
  uint8_t edgesPerDetent() const
  {
    return _edgesPerDetent;
  }

  uint8_t reversalSlop() const
  {
    return _reversalSlop;
  }

  uint8_t edgeDebounceMs() const
  {
    return _edgeDebounceMs;
  }

  uint8_t btnDebounceMs() const
  {
    return _btnDebounceMs;
  }

  void setBtnDebounceMs(uint8_t btnDebounceMs)
  {
    _btnDebounceMs = btnDebounceMs;
  }

private:
  uint8_t _edgesPerDetent = 2;
  uint8_t _reversalSlop   = 1;
  uint8_t _edgeDebounceMs = 5;
  uint8_t _btnDebounceMs  = 20;

  // ISR: read pins, enqueue 2-bit state. No branches, no literals.
  static void IRAM_ATTR isrLog(void* arg) {
    auto* s = static_cast<RotaryEncoder*>(arg);
    const uint8_t curr = (gpio_get_level(s->_clk) << 1) | gpio_get_level(s->_dt);
    const uint8_t w = s->_rbW;
    const uint8_t n = (uint8_t)((w + 1) & s->_rbMask);
    if (n != s->_rbR) { s->_rb[w] = curr; s->_rbW = n; } // drop if full
  }

  const gpio_num_t _clk, _dt, _sw;

  // ring buffer for ISR→task handoff
  static constexpr uint8_t _rbSize = 32;
  static constexpr uint8_t _rbMask = _rbSize - 1; // 31
  volatile uint8_t _rb[_rbSize] = {};
  volatile uint8_t _rbW = 0, _rbR = 0;

  // decoder state (task context)
  uint8_t  _prev = 0;
  int8_t   _accum = 0;
  uint32_t _lastEdgeUs = 0;

  // button debounce
  bool _btnLastSample=false, _btnStable=false;
  uint32_t _btnLastMs=0;

  StepCb  _onStep=nullptr;
  ClickCb _onClick=nullptr;
};
