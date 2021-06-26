#pragma once

// Start generating a PWM signal on pin PB2, with the given frequency and duty cycle.
void startPB2PWM(long frequencyHz, int dutyCyclePercent);

// Stop generating a PWM signal on pin PB2
void stopPB2PWM();

