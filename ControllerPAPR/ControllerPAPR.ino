//================================================================
// OpenPAPR Controller Firmware
// ---
// Author: Aaron Sun

#include <FanController.h>
#include <ButtonDebounce.h>

//================================================================
// OUTPUT PINS
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
#define POWER_OFF_PIN                13 // Power

//================================================================
// INPUT PINS
#define FAN_TACHOMETER_PIN          2 // FIXED
#define BATTERY_VOLTAGE_PIN         A1
#define BUTTON_2_PIN                12 // Lower
#define BUTTON_3_PIN                11 // Higher

//================================================================
// CONSTANTS
#define FAN_TACHOMETER_PULSES_PER_ROTATION    2

// Voltage divider as a ratio
#define BATTERY_VOLTAGE_DIVIDER_NUMERATOR     1       // This will be the pull down resistor value
#define BATTERY_VOLTAGE_DIVIDER_DENOMINATOR   (uint32_t)5       // This is the cumulative resitance of both the pull up and pull down

#define BATTERY_CELLS_SERIES                    5
#define BATTERY_VOLTAGE_DENOMINATOR           100     // Divide all voltages by this number
#define BATTERY_VOLTAGE_100_PERCENT           (420 * BATTERY_CELLS_SERIES)     // s * 4.20V (for LiPo with 0.2C)
#define BATTERY_VOLTAGE_75_PERCENT            (390 * BATTERY_CELLS_SERIES)     // s * 3.90V
#define BATTERY_VOLTAGE_50_PERCENT            (375 * BATTERY_CELLS_SERIES)     // s * 3.75V
#define BATTERY_VOLTAGE_25_PERCENT            (370 * BATTERY_CELLS_SERIES)     // s * 3.70V
#define BATTERY_VOLTAGE_CRITICAL              (350 * BATTERY_CELLS_SERIES)     // s * 3.50V

const uint32_t BATTERY_VOLTAGE_0_THRESHOLD       = ((uint32_t)BATTERY_VOLTAGE_CRITICAL);
const uint32_t BATTERY_VOLTAGE_25_THRESHOLD      = ((uint32_t)BATTERY_VOLTAGE_25_PERCENT);
const uint32_t BATTERY_VOLTAGE_50_THRESHOLD      = ((uint32_t)BATTERY_VOLTAGE_50_PERCENT);
const uint32_t BATTERY_VOLTAGE_75_THRESHOLD      = ((uint32_t)BATTERY_VOLTAGE_75_PERCENT);
const uint32_t BATTERY_VOLTAGE_100_THRESHOLD     = ((uint32_t)BATTERY_VOLTAGE_100_PERCENT);

#define BATTERY_LEVEL_CRITICAL  0
#define BATTERY_LEVEL_LOW       1
#define BATTERY_LEVEL_MEDIUM    2
#define BATTERY_LEVEL_HIGH      3
#define BATTERY_LEVEL_FULL      4

// Choose a threshold in milliseconds between readings. A smaller value will give more
// updated results, while a higher value will give more accurate and smooth readings.
#define FAN_THRESHOLD           1000

//================================================================
// PROGRAM

// TODO: debounds and hold times are different properties. Unpress debound time should
// be shorter.Is there
//ButtonDebounce button1(BUTTON_1_PIN, 500);
ButtonDebounce button2(BUTTON_2_PIN, 100);
ButtonDebounce button3(BUTTON_3_PIN, 100);
FanController fan(FAN_TACHOMETER_PIN, FAN_THRESHOLD, FAN_PWM_PIN);

uint8_t s_fan_level = 0;
uint8_t s_battery_level = 0;
uint32_t s_battery_level_start = 0;
bool s_error = false;
bool s_on = true;

uint8_t s_fan_pwm_low = 60;
uint8_t s_fan_pwm_mid = 80;
uint8_t s_fan_pwm_hi = 100;

/**
 * Program setup.
 */
void setup() {
  
  // Setup input pins.
  pinMode(BATTERY_VOLTAGE_PIN, INPUT);

  // Setup output pins.
  pinMode(MODE_LED_1_PIN, OUTPUT);
  pinMode(MODE_LED_2_PIN, OUTPUT);
  pinMode(MODE_LED_3_PIN, OUTPUT);
  pinMode(BATTERY_LED_1_PIN, OUTPUT);
  pinMode(BATTERY_LED_2_PIN, OUTPUT);
  pinMode(BATTERY_LED_3_PIN, OUTPUT);
  pinMode(BATTERY_LED_4_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  pinMode(POWER_OFF_PIN, OUTPUT);
  digitalWrite(POWER_OFF_PIN, LOW);
  
  //button1.setCallback(onButton1Change);
  button2.setCallback(onButton2Change);
  button3.setCallback(onButton3Change);
  fan.begin();
  
  Serial.begin(115200);
  Serial.println("PAPR loaded");

  updateFanLed();
}

void updateFanLed() {
  digitalWrite(MODE_LED_3_PIN, (s_fan_level > 1) ? HIGH : LOW);
  digitalWrite(MODE_LED_2_PIN, (s_fan_level > 0) ? HIGH : LOW);
  digitalWrite(MODE_LED_1_PIN, HIGH);
  Serial.println("Fan: " + String(s_fan_level));

  switch (s_fan_level) {
    case 0:
      fan.setDutyCycle(s_fan_pwm_low);
      break;
    case 1:
      fan.setDutyCycle(s_fan_pwm_mid);
      break;
    case 2:
      fan.setDutyCycle(s_fan_pwm_hi);
      break;
  }
}

/** Power. */
void onButton1Change(const int state) {
  Serial.println("Button1: " + String(state));
  if (state == HIGH) {
    s_on = !s_on;

    if (s_on) {
      updateFanLed();
    } else {
      fan.setDutyCycle(0);
      
      digitalWrite(MODE_LED_3_PIN, LOW);
      digitalWrite(MODE_LED_2_PIN, LOW);
      digitalWrite(MODE_LED_1_PIN, LOW);
      digitalWrite(BATTERY_LED_1_PIN, LOW);
      digitalWrite(BATTERY_LED_2_PIN, LOW);
      digitalWrite(BATTERY_LED_3_PIN, LOW);
      digitalWrite(BATTERY_LED_4_PIN, LOW);
      digitalWrite(BUZZER_PIN, LOW);
      digitalWrite(VIBRATOR_PIN, LOW);
    }
  }
}

/** Lower Fan Speed. */
void onButton2Change(const int state) {
  Serial.println("Button2: " + String(state));
  if (state == HIGH) {
    if (s_fan_level > 0) {
      s_fan_level--; 
    }
    updateFanLed();
  }
}

/** Raise Fan Speed. */
void onButton3Change(const int state) {
  Serial.println("Button3: " + String(state));
  if (state == HIGH) {
    if (s_fan_level < 2) {
      s_fan_level++; 
    }
    updateFanLed();
  }
}

/**
 * Program loop.
 */
void loop() {

  //button1.update();
  if (!s_on) {
    return;
  }
  
  // Read inputs
  uint32_t current_time = millis();
  uint16_t battery_voltage = analogRead(BATTERY_VOLTAGE_PIN) * 5 * BATTERY_VOLTAGE_DENOMINATOR * BATTERY_VOLTAGE_DIVIDER_DENOMINATOR / BATTERY_VOLTAGE_DIVIDER_NUMERATOR / 512;

  // Loop subfunctions.
  loop_battery_voltage(current_time, battery_voltage);

  button2.update();
  button3.update();

  delay(10);
}

/**
 * Program loop: process battery voltage.
 */
void loop_battery_voltage(uint32_t current_time, uint32_t battery_voltage) {
  // Calculate battery indicator levels.

  // Lower the battery levels at the designated levels.
  switch (s_battery_level) {
    case BATTERY_LEVEL_FULL:
      if (battery_voltage < BATTERY_VOLTAGE_75_THRESHOLD) {
        s_battery_level = BATTERY_LEVEL_HIGH;
      }
      // Fall through.
    case BATTERY_LEVEL_HIGH:
      if (battery_voltage < BATTERY_VOLTAGE_50_THRESHOLD) {
        s_battery_level = BATTERY_LEVEL_MEDIUM;
      }
      // Fall through.
    case BATTERY_LEVEL_MEDIUM:
      if (battery_voltage < BATTERY_VOLTAGE_25_THRESHOLD) {
        s_battery_level = BATTERY_LEVEL_LOW;
      }
      // Fall through.
    case BATTERY_LEVEL_LOW:
      if (battery_voltage < BATTERY_VOLTAGE_0_THRESHOLD) {
        s_battery_level = BATTERY_LEVEL_CRITICAL;
        s_battery_level_start = current_time;
      }
      break;
    case BATTERY_LEVEL_CRITICAL:
      break;
    default:
      s_error = true;
      break;
  }

  // Raise the battery levels when above the midway mark to the next level.
  switch (s_battery_level) {
    case BATTERY_LEVEL_CRITICAL:
      if (battery_voltage > (BATTERY_VOLTAGE_0_THRESHOLD + BATTERY_VOLTAGE_25_THRESHOLD) / 2) {
        s_battery_level = BATTERY_LEVEL_LOW;
      }
      // Fall through.
    case BATTERY_LEVEL_LOW:
      if (battery_voltage > (BATTERY_VOLTAGE_25_THRESHOLD + BATTERY_VOLTAGE_50_THRESHOLD) / 2) {
        s_battery_level = BATTERY_LEVEL_MEDIUM;
      }
      // Fall through.
    case BATTERY_LEVEL_MEDIUM:
      if (battery_voltage > (BATTERY_VOLTAGE_50_THRESHOLD + BATTERY_VOLTAGE_75_THRESHOLD) / 2) {
        s_battery_level = BATTERY_LEVEL_HIGH;
      }
      // Fall through.
    case BATTERY_LEVEL_HIGH:
      if (battery_voltage > (BATTERY_VOLTAGE_75_THRESHOLD + BATTERY_VOLTAGE_100_THRESHOLD) / 2) {
        s_battery_level = BATTERY_LEVEL_FULL;
      }
      break;
    case BATTERY_LEVEL_FULL:
      break;
    default:
      s_error = true;
      break;
  }

  Serial.println("BATTERY: " + String(battery_voltage) + "v, :"+ String(s_battery_level)
    + " (" + String((uint32_t)BATTERY_VOLTAGE_0_THRESHOLD)
    + ", " + String(BATTERY_VOLTAGE_25_THRESHOLD)
    + ", " + String(BATTERY_VOLTAGE_50_THRESHOLD)
    + ", " + String(BATTERY_VOLTAGE_75_THRESHOLD)
    + ", " + String(BATTERY_VOLTAGE_100_THRESHOLD) + ")");

  // Blink the low battery LED if the battery level is critical.
  uint16_t led_indicator_1 = (s_battery_level >= BATTERY_LEVEL_LOW) ? HIGH : LOW;
  if (s_battery_level == BATTERY_LEVEL_CRITICAL) {
    led_indicator_1 = (current_time - s_battery_level_start % 1000 > 500) ? HIGH : LOW;
  }
  
  digitalWrite(BATTERY_LED_1_PIN, led_indicator_1);
  digitalWrite(BATTERY_LED_2_PIN, (s_battery_level >= BATTERY_LEVEL_MEDIUM) ? HIGH : LOW);
  digitalWrite(BATTERY_LED_3_PIN, (s_battery_level >= BATTERY_LEVEL_HIGH) ? HIGH : LOW);
  digitalWrite(BATTERY_LED_4_PIN, (s_battery_level >= BATTERY_LEVEL_FULL) ? HIGH : LOW);
  digitalWrite(BUZZER_PIN, (s_battery_level == BATTERY_LEVEL_CRITICAL) ? HIGH : LOW);
  digitalWrite(VIBRATOR_PIN, (s_battery_level == BATTERY_LEVEL_CRITICAL) ? HIGH : LOW);
}
