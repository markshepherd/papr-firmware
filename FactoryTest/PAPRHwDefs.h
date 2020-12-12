/*
*   PAPRHwDefs.h
* 
* This header file defines all the input/output pins on the v2 PAPR board,
* and provides code to configure the pins and initialize them to a default state.
*/
#pragma once

//================================================================
// OUTPUT PINS
#define FAN_PWM_PIN                 5   /* PD5 */  /* Analog 0-255 duty cycle */
#define BATTERY_LED_1_PIN           A0  /* PC0 */  /* LOW = on, HIGH = off */
#define BATTERY_LED_2_PIN           A1  /* PC1 */  /* LOW = on, HIGH = off */
#define BATTERY_LED_3_PIN           A2  /* PC2 */  /* LOW = on, HIGH = off */
#define Error_LED_PIN               A3  /* PC3 */  /* LOW = on, HIGH = off */
#define MODE_LED_1_PIN              A4  /* PC4 */  /* LOW = on, HIGH = off */
#define MODE_LED_2_PIN              A5  /* PC5 */  /* LOW = on, HIGH = off */
#define MODE_LED_3_PIN              0   /* PD0 */  /* LOW = on, HIGH = off */
#define BUZZER_PIN                  10  /* PB2 */
#define VIBRATOR_PIN                2   /* PD2 */

//================================================================
// INPUT PINS
#define BATTERY_VOLTAGE_PIN         A7  /* PC7, ADC7 */
#define Monitor_PIN                 7   /* PD7 */  /* LOW = power down button pushed */
#define FAN_RPM_PIN                 3   /* PD3 */  /* Digital square wave, frequency proportional to RPM */
#define FAN_UP_PIN                  1   /* PD1 */  /* LOW = pushed, HIGH = released */
#define FAN_DOWN_PIN                9   /* PB1 */  /* LOW = pushed, HIGH = released */

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

    // Turn off the USART, because it wants to use pins 0 and 1 (a.k.a. PORTD0 and PORTD1) which we are using for other things.
    // BTW we never call Serial.begin() so I don't know why the USART is on.
    bitClear(UCSR0B, RXEN0);
    bitClear(UCSR0B, TXEN0);
    bitClear(UCSR0B, RXCIE0);
    bitClear(UCSR0B, UDRIE0);
}

inline void initializeDevices() {
    // Fan to lowest speed
    analogWrite(FAN_PWM_PIN, 0);

    // All LEDs off
    PORTC = 0x3f;
    PORTD = 0x01;

    // Buzzer off
    analogWrite(BUZZER_PIN, 0);

    // Vibrator off
    digitalWrite(VIBRATOR_PIN, LOW);
}
