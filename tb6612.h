#pragma once

#include "output_pin.h"

// Represents one channel of motor control on the TB6612. Control
// the standby pin using an OutputPin<>.
template <typename IN1, typename IN2, typename PWM> class TB6612 {
public:
  TB6612() {
    pwm_.on();
    stop();
  }
  TB6612(const TB6612 &) = delete;
  TB6612 &operator=(const TB6612 &) = delete;

  void forward() {
    in1_.on();
    in2_.off();
  }

  void backward() {
    in1_.off();
    in2_.on();
  }

  void short_brake() {
    in1_.on();
    in2_.on();
  }

  void stop() {
    in1_.off();
    in2_.off();
  }

private:
  IN1 in1_;
  IN2 in2_;
  PWM pwm_;
};
