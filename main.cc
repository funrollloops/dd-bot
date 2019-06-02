#include <avr/cpufunc.h>
#include <avr/interrupt.h>
#include <avr/io.h>

#include "output_pin.h"
#include "tb6612.h"

#define CLOCK_RATE F_CPU
#define TIMER2_PRESCALER 64
#define TIMER2_INTERRUPTS_PER_SEC 1000
// Compute number of timer ticks between interrupts.
#define TIMER2_INITIAL \
    (UINT8_MAX - CLOCK_RATE/TIMER2_PRESCALER/TIMER2_INTERRUPTS_PER_SEC)

struct CLI {
  CLI() { cli(); }
} _;

// Attached peripherals, with configuration.
TimedOutput<OutputPinB<5>> builtin_led;
TB6612<OutputPinD<4>, OutputPinD<5>, OutputPinD<6>> motor_a;
TB6612<OutputPinD<7>, OutputPinB<0>, OutputPinB<1>> motor_b;

EMPTY_INTERRUPT(BADISR_vect)

#define RETI do { asm volatile("reti"); __builtin_unreachable(); } while (0)

static int16_t second = 0;
static int8_t which = 0;

ISR(TIMER2_OVF_vect, ISR_NAKED) {
  TCNT2 = TIMER2_INITIAL;
  builtin_led.tick();

  if (++second == 1000) {
    builtin_led.set_ticks(500);
    second = 0;
    ++which;

    switch (which & 0x3) {
      case 0:
        motor_a.forward();
        motor_b.forward();
        break;

      case 1:
        motor_a.stop();
        motor_b.stop();
        break;

      case 2:
        motor_a.backward();
        motor_b.backward();
        break;

      case 3:
        motor_a.short_brake();
        motor_b.short_brake();
        break;
    }
  }
  RETI;
}

void setup() {
  cli();  // Disable interrupts.
  // Set clock divisor to 1 to run at full 8MHz.
  CLKPR = 0x80;  // Enable setting clock divisor.
  _NOP();        // Ensure instructions are not re-ordered.
  CLKPR = 0x00;  // Set divisor to 1.

  // Configure TIMER2 to interrupt TIMER2_INTERRUPTS_PER_SEC.
  TCCR2A = 0;
  TCCR2B = 0;
  TCCR2B |= _BV(CS22);  // prescaler=64 (increment TCNT2 every 64 cycles).
  TCNT2 = TIMER2_INITIAL;  // Interrupt triggered when this reached UINT8_MAX.
  TIMSK2 |= _BV(TOIE2);  // Enable interrupts for timer2.

  sei(); // Enable interrups.
}

int main() {
  setup();
  // Don't bother with sleep / low power modes. This allows us
  // to use naked ISRs which do not save and restore state.
  while (true);
}
