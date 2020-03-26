// CONTROLLER PLATFORM
//
// Different controller platforms have different methods to drive 25kHz PWM.
#define ATMEGA                      1
#define ATTINY                      2

#if defined(__AVR_ATmega328P__)
#define CONTROLLER_PLATFORM         ATMEGA
#elif defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
#define CONTROLLER_PLATFORM         ATTINY
#endif

// OUTPUT PINS
#define FAN_PWM_PIN                 3
#define BUZZER_PIN                  13
#define ALERT_LED_PIN               12

// INPUT PINS
#define POTENTIOMETER_PIN           A1
#define BATTERY_VOLTAGE_PIN         A0

// CONSTANTS
#define BATTERY_VOLTAGE_MULTIPLIER  6 * 5
#define BATTERY_VOLTAGE_THRESHOLD   15 * 1024


void setup() {
  
  // Setup input pins.
  pinMode(POTENTIOMETER_PIN, INPUT);
  pinMode(BATTERY_VOLTAGE_PIN, INPUT);

  // Setup output pins.
  pinMode(FAN_PWM_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Setup PWM clock.
  beginPwm25kHz();
}

void loop() {
  // put your main code here, to run repeatedly:


  uint16_t battery_voltage = analogRead(BATTERY_VOLTAGE_PIN) * BATTERY_VOLTAGE_MULTIPLIER;
  uint16_t potentiometer = analogRead(POTENTIOMETER_PIN);

  setPwmDuty(min(potentiometer / 4, 0xff));

  uint16_t battery_alert = (battery_voltage < BATTERY_VOLTAGE_THRESHOLD) ? HIGH : LOW;
  digitalWrite(BUZZER_PIN, battery_alert);
  digitalWrite(ALERT_LED_PIN, battery_alert);

  delay(10);
}


// https://create.arduino.cc/projecthub/MyName1sSimon/control-pwm-fans-with-an-arduino-7bef86
// http://www.nomad.ee/micros/tiny_pwm/

/**
 * Setup the PWM for use with a 25kHz PWM fan.
 */
void beginPwm25kHz() {

#if CONTROLLER_PLATFORM == ATMEGA
  // Pin 3
  TCCR2A = 0;                               // TC2 Control Register A
  TCCR2B = 0;                               // TC2 Control Register B
  TIMSK2 = 0;                               // TC2 Interrupt Mask Register
  TIFR2 = 0;                                // TC2 Interrupt Flag Register

  // OC2B cleared/set on match when up/down counting, fast PWM
  TCCR2A |= (1 << COM2B1)
          | (1 << WGM21)
          | (1 << WGM20);  
  TCCR2B |= (1 << WGM22)
          | (1 << CS21);     // prescaler 8
  
  OCR2A = 79;                               // TOP overflow value (Hz)
  OCR2B = 0;
  
#elif CONTROLLER_PLATFORM == ATTINY

  #define PERIOD_uS_16MHz 2000

  // See http://www.technoblogy.com/show?LE0
  
  // Set timer 1 to interrupt at 1kHz [1000 us]
  TCCR1A = 0;                               // TC1 Control Register A
  TCCR1B = 0;                               // TC1 Control Register B
  TCNT1  = 0;                               // TC1 Counter Value

  // OCR1A = (16*10^6) / (1000[Hz]*8[/tick]) - 1 (must be <65536)
  OCR1A = PERIOD_uS_16MHz - 1;
  OCR1B = PERIOD_uS_16MHz + 1;
  
  TCCR1B |= (1 << WGM12); // turn on CTC mode (reset on compare)
  // TCCR1B |= (1 << CS11); // Set CS11 to slow 8x
  TCCR1B |= (1 << CS10); // Set CS10 to enable timer
  
  TIMSK1 |= (1 << OCIE1A); // enable rising edge timer compare interrupt
  TIMSK1 |= (1 << OCIE1B); // enable falling edge timer compare interrupt
#else
  #error beginPwm25kHz: CONTROLLER_PLATFORM invalid or absent.
#endif
  
}

/** 
 *  Sets the duty cycle of the PWM pin
 *  @param duty The target duty cycle from 0 to 255
 */
void setPwmDuty(byte duty) {
  
#if CONTROLLER_PLATFORM == ATMEGA
  OCR2B = duty;                             // PWM Width (duty)
#elif CONTROLLER_PLATFORM == ATTINY
  OCR1B = duty;
#else
  #error setPwmDuty: CONTROLLER_PLATFORM invalid or absent.
#endif
  
}
