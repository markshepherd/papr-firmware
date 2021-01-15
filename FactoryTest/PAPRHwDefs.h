/*
*   PAPRHwDefs.h
* 
* This header file defines all the input/output pins on the v2 PAPR board,
* and provides code to configure the pins, initialize the hardware to a default state,
* and interpret raw input values.
*/
#pragma once

//================================================================
// OUTPUT PINS
#define FAN_PWM_PIN                 5   /* PD5 */  /* Analog: 0-255 duty cycle */
#define BATTERY_LED_1_PIN           A0  /* PC0 */  /* Digital: LOW = on, HIGH = off */
#define BATTERY_LED_2_PIN           A1  /* PC1 */  /* Digital: LOW = on, HIGH = off */
#define BATTERY_LED_3_PIN           A2  /* PC2 */  /* Digital: LOW = on, HIGH = off */
#define Error_LED_PIN               A3  /* PC3 */  /* Digital: LOW = on, HIGH = off */
#define MODE_LED_1_PIN              A4  /* PC4 */  /* Digital: LOW = on, HIGH = off */
#define MODE_LED_2_PIN              A5  /* PC5 */  /* Digital: LOW = on, HIGH = off */
#define MODE_LED_3_PIN              0   /* PD0 */  /* Digital: LOW = on, HIGH = off */
#define BUZZER_PIN                  10  /* PB2 */  /* Analog: 0-255 duty cycle */
#define VIBRATOR_PIN                2   /* PD2 */  /* Digital: LOW = off, HIGH = on */

const int LED_ON = LOW;
const int LED_OFF = HIGH;

const int VIBRATOR_ON = HIGH;
const int VIBRATOR_OFF = LOW;

const int BUZZER_ON = 64; // I tried different numbers and this one sounded the best :)
const int BUZZER_OFF = 0;

//================================================================
// INPUT PINS
#define BATTERY_VOLTAGE_PIN         A7  /* PC7 */  /* Analog: values range from roughly 0x100 (6 volts) to 0x300 (24 volts) */
#define Monitor_PIN                 7   /* PD7 */  /* Digital: LOW = power down button pushed */
#define FAN_RPM_PIN                 3   /* PD3 */  /* Digital: square wave, frequency proportional to RPM */
#define FAN_UP_PIN                  1   /* PD1 */  /* Digital: LOW = pushed, HIGH = released */
#define FAN_DOWN_PIN                9   /* PB1 */  /* Digital: LOW = pushed, HIGH = released */

const int BUTTON_PUSHED = LOW;
const int BUTTON_RELEASED = HIGH;

//================================================================
inline void configurePins() {
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

inline void initializeDevices() {
    // Fan to lowest speed
    analogWrite(FAN_PWM_PIN, 0);

    // All LEDs off
    PORTC = 0x3f;
    PORTD = 0x01;

    // Buzzer off
    analogWrite(BUZZER_PIN, BUZZER_OFF);
}

// Return battery fullness as a number between 0 (empty = 12 volts) and 100 (full = 24 volts).
inline unsigned int readBatteryFullness() {
    // Here are the battery readings we expect for minimum and maximum battery voltages. These numbers were determined empirically.
    const int readingAt12Volts = 386;
    const int readingAt24Volts = 784;

    // TODO: get rid of the hard-coded constants, and use a better way to derive the battery readings.
    // There is code at https://github.com/airtoall/papr-firmware/blob/752f2db06e0bd756dd24af65175921b3bed734dd/ControllerFirmware/ControllerFirmware.ino#L196,
    // but I'm not sure what it means.

    // Read the current battery voltage and limit the value to the expected range.
    uint16_t reading = analogRead(BATTERY_VOLTAGE_PIN);
    if (reading < readingAt12Volts) reading = readingAt12Volts;
    if (reading > readingAt24Volts) reading = readingAt24Volts;

    // Calculate how full the battery is. This will be a number between 0 and 1.
    // We use floating point because it's easier than trying to do this in fixed point, and the program memory impact appears to be small.
    float fullness = float(reading - readingAt12Volts) / float(readingAt24Volts - readingAt12Volts);
    return (unsigned int)(fullness * 100);
}
