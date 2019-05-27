#include <avr/cpufunc.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdio.h>

#include "output_pin.h"

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

struct __attribute__((packed)) PosPair { int16_t first, second; };

EMPTY_INTERRUPT(BADISR_vect)

#define RETI do { asm volatile("reti"); __builtin_unreachable(); } while (0)

static int16_t max_flash = 100;
static int16_t flash = 0;

ISR(TIMER2_OVF_vect, ISR_NAKED) {
  TCNT2 = TIMER2_INITIAL;
  builtin_led.tick();

  if (flash-- == 0) {
    builtin_led.set_ticks(max_flash);
    max_flash <<= 1;
    flash = max_flash;
  }
  RETI;
}

void setup() {
  cli();  // Disable interrupts.
  // Set clock divisor to 1 to run at full 8MHz.
  CLKPR = 0x80;  // Enable setting clock divisor.
  _NOP();        // Ensure instructions are not re-ordered.
  CLKPR = 0x00;  // Set divisor to 1.

  // Configure TIMER2 to interrupt when TCNT2 reaches UINT8_MAX.
  TCCR2A = 0;
  TCCR2B = 0;
  TCCR2B |= _BV(CS22);  // set prescaler to 64.
  TCNT2 = TIMER2_INITIAL;
  TIMSK2 |= _BV(TOIE2);

  // Enable SPI for read and write.
  DDRB |= _BV(4);  // Set MISO pin to output.
  SPCR |= _BV(SPE);  // Enable SPI in slave mode.
  SPCR |= _BV(SPIE);  // Enable SPI interrupts.

  // Put status register into SPDR on reset to help debug.
  SPDR = MCUSR;
  MCUSR = 0;
  sei(); // Enable interrups.
}

int main() {
  setup();
  // Don't bother with sleep / low power modes. This allows us
  // to use naked ISRs which do not save and restore state.
  while (true);
}
