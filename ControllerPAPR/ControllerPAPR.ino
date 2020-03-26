
#define FAN_PWM_PIN                 3
#define BUZZER_PIN                  13
#define ALERT_LED_PIN               12

#define POTENTIOMETER_PIN           A1
#define BATTERY_VOLTAGE_PIN         A0
#define BATTERY_VOLTAGE_MULTIPLIER  6 * 5
#define BATTERY_VOLTAGE_THRESHOLD   15 * 1024




void setup() {
  // put your setup code here, to run once:

  pinMode(POTENTIOMETER_PIN, INPUT);
  pinMode(BATTERY_VOLTAGE_PIN, INPUT);

  pinMode(FAN_PWM_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

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
void beginPwm25kHz() {
  TCCR2A = 0;                               // TC2 Control Register A
  TCCR2B = 0;                               // TC2 Control Register B
  TIMSK2 = 0;                               // TC2 Interrupt Mask Register
  TIFR2 = 0;                                // TC2 Interrupt Flag Register
  TCCR2A |= (1 << COM2B1) | (1 << WGM21) | (1 << WGM20);  // OC2B cleared/set on match when up/down counting, fast PWM
  TCCR2B |= (1 << WGM22) | (1 << CS21);     // prescaler 8
  OCR2A = 79;                               // TOP overflow value (Hz)
  OCR2B = 0;
}

// out of 255
void setPwmDuty(byte ocrb) {
  OCR2B = ocrb;                             // PWM Width (duty)
}
