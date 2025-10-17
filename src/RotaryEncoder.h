#pragma once
#include <Arduino.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class RotaryEncoder {
public:
  using StepCb  = void(*)(int8_t dir); // +1 CW, -1 CCW
  using ClickCb = void(*)();

  RotaryEncoder(gpio_num_t clk, gpio_num_t dt, gpio_num_t sw)
  : _clk(clk), _dt(dt), _sw(sw) {}

  // tuning
  void setEdgesPerDetent(uint8_t n)  { _edgesPerDetent = (n==1||n==2||n==4)? n:4; }
  void setEdgeDebounceMs(uint8_t ms) { _edgeDebounceMs = ms; }       // 0–3 typical
  void setButtonDebounceMs(uint8_t ms){ _btnDebounceMs = ms; }       // 10–30 typical
  void setReversalSlop(uint8_t e)    { _reversalSlop = (e<=3)? e:1; }

  void begin(StepCb onStep=nullptr, ClickCb onClick=nullptr) {
    _onStep = onStep; _onClick = onClick;

    pinMode((uint8_t)_clk, INPUT_PULLUP);
    pinMode((uint8_t)_dt,  INPUT_PULLUP);
    pinMode((uint8_t)_sw,  INPUT_PULLUP);

    // precompute regs/masks for ISR (no literals there)
    if ((int)_clk < 32) { _clkReg=&GPIO.in;      _clkMask=1u<< (int)_clk; }
    else                { _clkReg=&GPIO.in1.val; _clkMask=1u<<((int)_clk-32); }
    if ((int)_dt  < 32) { _dtReg =&GPIO.in;      _dtMask =1u<< (int)_dt;  }
    else                { _dtReg =&GPIO.in1.val; _dtMask =1u<<((int)_dt -32); }

    _prev = (fastRead(_clkReg,_clkMask)<<1) | fastRead(_dtReg,_dtMask);

    // CHANGE on both lines; ISR only queues samples
    attachInterruptArg((uint8_t)_clk, isrLog, this, CHANGE);
    attachInterruptArg((uint8_t)_dt,  isrLog, this, CHANGE);
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
      if (a >= +thresh) { _accum = 0; if (_onStep) _onStep(+1); }
      else if (a <= -thresh) { _accum = 0; if (_onStep) _onStep(-1); }
      else { _accum = a; }
    }

    // button debounce (active-low)
    const uint32_t ms = millis();
    const bool sample = (gpio_get_level(_sw) == 0);
    if (sample != _btnLastSample) { _btnLastSample = sample; _btnLastMs = ms; }
    if ((ms - _btnLastMs) >= _btnDebounceMs && sample != _btnStable) {
      _btnStable = sample;
      if (_btnStable && _onClick) _onClick();
    }
  }


public:
  uint8_t edgesPerDetent() const
  {
    return _edgesPerDetent;
  }

  void setEdgesPerDetent1(uint8_t edgesPerDetent)
  {
    _edgesPerDetent = edgesPerDetent;
  }

  uint8_t reversalSlop() const
  {
    return _reversalSlop;
  }

  void setReversalSlop1(uint8_t reversalSlop)
  {
    _reversalSlop = reversalSlop;
  }

  uint8_t edgeDebounceMs() const
  {
    return _edgeDebounceMs;
  }

  void setEdgeDebounceMs1(uint8_t edgeDebounceMs)
  {
    _edgeDebounceMs = edgeDebounceMs;
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
  uint8_t _edgesPerDetent = 4;
  uint8_t _reversalSlop   = 1;
  uint8_t _edgeDebounceMs = 1;
  uint8_t _btnDebounceMs  = 20;

  // IRAM-safe bit read for ISR setup
  static inline int IRAM_ATTR fastRead(volatile uint32_t* reg, uint32_t mask) { return (*reg & mask) ? 1 : 0; }

  // ISR: read pins, enqueue 2-bit state. No branches, no literals.
  static void IRAM_ATTR isrLog(void* arg) {
    auto* s = static_cast<RotaryEncoder*>(arg);
    const uint8_t curr = ( ((*s->_clkReg & s->_clkMask)?1:0) << 1 ) | ((*s->_dtReg & s->_dtMask)?1:0);
    const uint8_t w = s->_rbW;
    const uint8_t n = (uint8_t)((w + 1) & s->_rbMask);
    if (n != s->_rbR) { s->_rb[w] = curr; s->_rbW = n; } // drop if full
  }

  const gpio_num_t _clk, _dt, _sw;

  // precomputed for ISR
  volatile uint32_t* _clkReg=nullptr; uint32_t _clkMask=0;
  volatile uint32_t* _dtReg =nullptr; uint32_t _dtMask =0;

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
