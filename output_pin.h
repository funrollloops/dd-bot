#ifndef FUNROLLLOOPS_ARDUINO_OUTPUT_PIN_H_
#define FUNROLLLOOPS_ARDUINO_OUTPUT_PIN_H_

#include <avr/io.h>

#include "attributes.h"

// Using native AVR names.
#define OUTPUT_PORT(suffix) \
  template <int kPin> class OutputPin##suffix { \
    public: \
      OutputPin##suffix() ALWAYS_INLINE { DDR##suffix |= _BV(kPin); } \
      static void on() ALWAYS_INLINE { PORT##suffix |= _BV(kPin); } \
      static void off() ALWAYS_INLINE { PORT##suffix &= ~_BV(kPin); } \
      bool operator=(bool value) ALWAYS_INLINE { value ? on() : off(); return value; } \
    }

OUTPUT_PORT(B);
OUTPUT_PORT(C);
OUTPUT_PORT(D);
#undef OUTPUT_PORT

template <typename Pin>
class TimedOutput {
 public:
  TimedOutput() { pin_.off(); }

  // To avoid races, tick() and set_ticks() should not called
  // with interrupts enabled.
  void tick() ALWAYS_INLINE {
    auto ticks = ticks_;
    if (ticks == 0) return;
    if (--ticks == 0) pin_.off();
    ticks_ = ticks;
  }

  void set_ticks(uint16_t ticks) ALWAYS_INLINE {
    pin_ = ticks > 0;
    ticks_ = ticks;
  }

 private:
  Pin pin_;
  volatile uint16_t ticks_ = 0;
};

#endif
