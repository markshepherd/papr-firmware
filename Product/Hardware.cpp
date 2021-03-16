/*
 * Hardware.cpp
 */
#include "Hardware.h"
#include "PAPRHwDefs.h"
#include "MySerial.h"
#include <avr/interrupt.h>

/********************************************************************
 * PAPR-specific functions
 ********************************************************************/

// Configure all the microcontroller IO pins that this app uses.
void Hardware::configurePins()
{
    // Output pins are defined by a 1 bit
    DDRB = DDRB | B00000100; // Outputs are PB2
    DDRC = DDRC | B00111111; // Outputs are PC0, PC1, PC2, PC3, PC4, PC5
    DDRD = DDRD | B00100101; // Outputs are PD0, PD2, PD5

    // Input pins are defined by a 0 bit
    DDRB = DDRB & B11111101; // Inputs are PB1
    DDRC = DDRC & B01111111; // Inputs are PC7
    DDRD = DDRD & B01110101; // Inputs are PD1, PD3, PD7

    // PD7 needs a pullup resistor
    PORTD = B10000000;

    // If we were launched in Debug mode from Visual Micro, then the UART is probably enabled.
    // Turn off the USART, because it wants to use pins 0 and 1 (a.k.a. PORTD0 and PORTD1) which we are using for other things.
    //bitClear(UCSR0B, RXEN0);
    //bitClear(UCSR0B, TXEN0);
    //bitClear(UCSR0B, RXCIE0);
    //bitClear(UCSR0B, UDRIE0);
}

// Set all devices to an initial state
void Hardware::initializeDevices()
{
    // Fan to lowest speed
    analogWrite(FAN_PWM_PIN, 0);

    // All LEDs off
    PORTC = 0x3f;
    PORTD = 0x01;

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

void (*powerButtonInterruptCallback) ();

#ifndef SERIAL_ENABLED
ISR(PCINT2_vect)
{
    powerButtonInterruptCallback();
}
#endif

void Hardware::setPowerButtonInterruptCallback(void (*callback) ())
{
    powerButtonInterruptCallback = callback;
 
    // Enable Pin Change Interrupt for the Power On button.
    PCMSK2 |= 0x80; // set PCINT23 = 1 to enable PCINT on pin PD7
    PCICR |= 0x04; // set PCIE2 = 1 to enable PC interrupts
}
