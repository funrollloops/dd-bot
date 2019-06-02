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
OutputPinD<4> ain1;
OutputPinD<5> ain2;
OutputPinD<6> apwm;

OutputPinD<7> bin1;
OutputPinB<0> bin2;
OutputPinB<1> bpwm;

EMPTY_INTERRUPT(BADISR_vect)

#define RETI do { asm volatile("reti"); __builtin_unreachable(); } while (0)

static int16_t max_flash = 100;
static int16_t flash = 0;

static int16_t second = 0;
static int8_t which;

ISR(TIMER2_OVF_vect, ISR_NAKED) {
  TCNT2 = TIMER2_INITIAL;
  builtin_led.tick();

  if (flash-- == 0) {
    max_flash <<= 1;
    flash = max_flash;
  }

  if (++second == 1000) {
    builtin_led.set_ticks(500);
    second = 0;
    ++which;
    ain1 = which & 1;
    ain2 = which & 2;
    bin1 = which & 4;
    bin2 = which & 8;
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

  apwm.on();
  bpwm.on();

  ain1.on();
  ain2.off();

  bin1.on();
  bin2.off();
  sei(); // Enable interrups.
}

int main() {
  setup();
  // Don't bother with sleep / low power modes. This allows us
  // to use naked ISRs which do not save and restore state.
  while (true);
}
