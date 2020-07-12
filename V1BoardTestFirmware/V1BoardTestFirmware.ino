#include <FanController.h>

#define FAN_PWM_PIN                 9 // FIXED
#define BATTERY_LED_1_PIN           3
#define BATTERY_LED_2_PIN           4
#define BATTERY_LED_3_PIN           5
#define BATTERY_LED_4_PIN           6
#define MODE_LED_1_PIN              7
#define MODE_LED_2_PIN              8
#define MODE_LED_3_PIN              10
#define BUZZER_PIN                  A5
#define VIBRATOR_PIN                A4
#define POWER_OFF_PIN               13 // Power

#define FAN_TACHOMETER_PIN          2 // FIXED
#define FAN_THRESHOLD               1000

uint8_t s_fan_pwm_off = 0;
uint8_t s_fan_pwm_low = 60;
uint8_t s_fan_pwm_mid = 80;
uint8_t s_fan_pwm_hi = 100;
uint8_t s_fan_pwm_full = 255;

FanController fan(FAN_TACHOMETER_PIN, FAN_THRESHOLD, FAN_PWM_PIN);

void setup() {
  // Setup the LED pins as outputs and turn the LEDs off
  pinMode(BATTERY_LED_1_PIN, OUTPUT);
  digitalWrite(BATTERY_LED_1_PIN, HIGH);
  
  pinMode(BATTERY_LED_2_PIN, OUTPUT);
  digitalWrite(BATTERY_LED_2_PIN, HIGH);
  
  pinMode(BATTERY_LED_3_PIN, OUTPUT);
  digitalWrite(BATTERY_LED_3_PIN, HIGH);
  
  pinMode(BATTERY_LED_4_PIN, OUTPUT);
  digitalWrite(BATTERY_LED_4_PIN, HIGH);

  pinMode(MODE_LED_1_PIN, OUTPUT);
  digitalWrite(MODE_LED_1_PIN, HIGH);
  
  pinMode(MODE_LED_2_PIN, OUTPUT);
  digitalWrite(MODE_LED_2_PIN, HIGH);
  
  pinMode(MODE_LED_3_PIN, OUTPUT);
  digitalWrite(MODE_LED_3_PIN, HIGH);

  // Buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  analogWrite(BUZZER_PIN, 0);
  
  // Vibrator
  pinMode(VIBRATOR_PIN, OUTPUT);
  analogWrite(VIBRATOR_PIN, 0);

  fan.setDutyCycle(s_fan_pwm_full);
}

void loop() {
  // BATTERY LEDS
  digitalWrite(BATTERY_LED_1_PIN, LOW);
  delay(1000);
  digitalWrite(BATTERY_LED_1_PIN, HIGH);
  digitalWrite(BATTERY_LED_2_PIN, LOW);
  delay(1000);
  digitalWrite(BATTERY_LED_2_PIN, HIGH);
  digitalWrite(BATTERY_LED_3_PIN, LOW);
  delay(1000);
  digitalWrite(BATTERY_LED_3_PIN, HIGH);
  digitalWrite(BATTERY_LED_4_PIN, LOW);
  delay(1000);
  digitalWrite(BATTERY_LED_4_PIN, HIGH);

  // MODE LEDS
  digitalWrite(MODE_LED_1_PIN, LOW);
  delay(1000);
  digitalWrite(MODE_LED_1_PIN, HIGH);
  digitalWrite(MODE_LED_2_PIN, LOW);
  delay(1000);
  digitalWrite(MODE_LED_2_PIN, HIGH);
  digitalWrite(MODE_LED_3_PIN, LOW);
  delay(1000);
  digitalWrite(MODE_LED_3_PIN, HIGH);
  delay(1000);
  digitalWrite(MODE_LED_3_PIN, HIGH);

  // BUZZER
  analogWrite(BUZZER_PIN, 128);
  delay(1000);
  analogWrite(BUZZER_PIN, 0);

  // VIBRATOR
  analogWrite(VIBRATOR_PIN, 128);
  delay(1000);
  analogWrite(VIBRATOR_PIN, 0);

  // FAN LOW
  analogWrite(FAN_PWM_PIN, s_fan_pwm_low);
  digitalWrite(MODE_LED_1_PIN, LOW);
  delay(5000);
  
  // FAN MED
  analogWrite(FAN_PWM_PIN, s_fan_pwm_mid);
  digitalWrite(MODE_LED_1_PIN, LOW);
  digitalWrite(MODE_LED_2_PIN, LOW);
  delay(5000);
  
  // FAN HIGH
  analogWrite(FAN_PWM_PIN, s_fan_pwm_hi);
  digitalWrite(MODE_LED_1_PIN, LOW);
  digitalWrite(MODE_LED_2_PIN, LOW);
  digitalWrite(MODE_LED_3_PIN, LOW);
  delay(5000);

  // FAN OFF
  analogWrite(FAN_PWM_PIN, s_fan_pwm_off);
  digitalWrite(MODE_LED_1_PIN, HIGH);
  digitalWrite(MODE_LED_2_PIN, HIGH);
  digitalWrite(MODE_LED_3_PIN, HIGH);
}
