/*
 * Hardware.cpp
 */
#include "Hardware.h"
#include <avr/interrupt.h>

Hardware::Hardware() :powerOnButtonInterruptCallback(0), fanRPMInterruptCallback(0) { }

Hardware Hardware::instance;

/********************************************************************
 * PAPR-specific functions
 ********************************************************************/

// Configure all the microcontroller IO pins that this app uses.
void Hardware::configurePins()
{
    pinMode(FAN_UP_PIN, INPUT_PULLUP);
    pinMode(FAN_DOWN_PIN, INPUT_PULLUP);
    pinMode(POWER_OFF_PIN, INPUT_PULLUP);
    pinMode(POWER_ON_PIN, INPUT_PULLUP);
    pinMode(FAN_PWM_PIN, OUTPUT);
    pinMode(FAN_RPM_PIN, INPUT);
    pinMode(FAN_ENABLE_PIN, OUTPUT);
    pinMode(BATTERY_VOLTAGE_PIN, INPUT);
    pinMode(CHARGE_CURRENT_PIN, INPUT);
    pinMode(REFERENCE_VOLTAGE_PIN, INPUT);
    pinMode(BOARD_POWER_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(BATTERY_LED_LOW_PIN, OUTPUT);
    pinMode(BATTERY_LED_MED_PIN, OUTPUT);
    pinMode(BATTERY_LED_HIGH_PIN, OUTPUT);
    pinMode(CHARGING_LED_PIN, OUTPUT);
    pinMode(FAN_LOW_LED_PIN, OUTPUT);
    pinMode(FAN_MED_LED_PIN, OUTPUT);
    pinMode(FAN_HIGH_LED_PIN, OUTPUT);
    pinMode(SERIAL_RX_PIN, INPUT);
    pinMode(SERIAL_TX_PIN, OUTPUT);
}

// Set all devices to an initial state
void Hardware::initializeDevices()
{
    // Fan on at lowest speed
    digitalWrite(FAN_ENABLE_PIN, FAN_ON);
    analogWrite(FAN_PWM_PIN, 0);

    // All LEDs off
    digitalWrite(BATTERY_LED_LOW_PIN, LED_OFF);
    digitalWrite(BATTERY_LED_MED_PIN, LED_OFF);
    digitalWrite(BATTERY_LED_HIGH_PIN, LED_OFF);
    digitalWrite(CHARGING_LED_PIN, LED_OFF);
    digitalWrite(FAN_LOW_LED_PIN, LED_OFF);
    digitalWrite(FAN_MED_LED_PIN, LED_OFF);
    digitalWrite(FAN_HIGH_LED_PIN, LED_OFF);

    // Buzzer off
    analogWrite(BUZZER_PIN, BUZZER_OFF);
}

int Hardware::watchdogStartup(void)
{
    int result = MCUSR;
    MCUSR = 0;
    wdt_disable();
    return result;
}

// prescalerSelect is 0..8, giving division factor of 1..256
void Hardware::setClockPrescaler(int prescalerSelect)
{
    noInterrupts();
    CLKPR = (1 << CLKPCE);
    CLKPR = prescalerSelect;
    interrupts();
}

void Hardware::reset()
{
    // "onReset" points to the RESET interrupt handler at address 0.
    void(*onReset) (void) = 0;
    onReset();
}

void Hardware::handleInterrupt() {
    if (powerOnButtonInterruptCallback) {
        unsigned int newPowerOnButtonState = digitalRead(POWER_ON_PIN);
        if (newPowerOnButtonState != powerOnButtonState) {
            powerOnButtonState = newPowerOnButtonState;
            powerOnButtonInterruptCallback->callback();
        }
    }

    if (fanRPMInterruptCallback) {
        unsigned int newFanRPMState = digitalRead(FAN_RPM_PIN);
        if (newFanRPMState != fanRPMState) {
            fanRPMState = newFanRPMState;
            fanRPMInterruptCallback->callback();
        }
    }
}

ISR(PCINT2_vect)
{
    Hardware::instance.handleInterrupt();
}

void Hardware::updateInterruptHandling() {
    // If anyone is interested in Power On button presses or Fan RPM signals, 
    // then here is where we set up handling for the appropriate Pin Change interrupts.
    // By default, PCMSK2 and PCICR are both 0, so we won't receive any Pin Change interrupts.
    powerOnButtonState = digitalRead(POWER_ON_PIN);
    fanRPMState = digitalRead(FAN_RPM_PIN);

    if (powerOnButtonInterruptCallback) {
        PCMSK2 |=   1 << PCINT23;  // set PCINT23 = 1 to enable PCINT on pin PD7
    } else {
        PCMSK2 &= ~(1 << PCINT23); // set PCINT23 = 0 to disable PCINT on pin PD7
    }

    if (fanRPMInterruptCallback) {
        PCMSK2 |=   1 << PCINT21;  // set PCINT21 = 1 to enable PCINT on pin PD5
    } else {
        PCMSK2 &= ~(1 << PCINT21); // set PCINT21 = 0 to disable PCINT on pin PD5
    }

    if (powerOnButtonInterruptCallback || fanRPMInterruptCallback) {
        PCICR |=   1 << PCIE2;     // set PCIE2 = 1 to enable PC interrupts
    } else {
        PCICR &= ~(1 << PCIE2);    // set PCIE2 = 0 to disable PC interrupts
    }
}

void Hardware::setPowerOnButtonInterruptCallback(InterruptCallback* callback)
{
    powerOnButtonInterruptCallback = callback;
    updateInterruptHandling();
}

void Hardware::setFanRPMInterruptCallback(InterruptCallback* callback)
{
    fanRPMInterruptCallback = callback;
    updateInterruptHandling();
}

void Hardware::setPowerMode(PowerMode mode)
{
    if (mode == fullPowerMode) {
        // Set the PCB to Full Power mode.
        digitalWrite(BOARD_POWER_PIN, BOARD_POWER_ON);

        // Wait for things to settle down
        delay(10);

        // Set the clock prescaler to give the max speed.
        setClockPrescaler(0);

        // We are now running at full power, full speed.
    } else {
        // Full speed doesn't work in low power mode, so drop the MCU clock speed to 1 MHz (8 MHz internal oscillator divided by 2**3). 
        setClockPrescaler(3);

        // Now we can enter low power mode,
        digitalWrite(BOARD_POWER_PIN, BOARD_POWER_OFF);

        // We are now running at low power, low speed.
    }
}

unsigned long getMillis()
{
    return Hardware::instance.millis();
}
