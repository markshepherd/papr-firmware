
//================================================================
// OUTPUT PINS
#define FAN_PWM_PIN                 5 
#define BATTERY_LED_1_PIN           A0
#define BATTERY_LED_2_PIN           A1
#define BATTERY_LED_3_PIN           A2
#define Error_LED_PIN               A3
#define MODE_LED_1_PIN              A4
#define MODE_LED_2_PIN              A5
#define MODE_LED_3_PIN              0
#define BUZZER_PIN                  10
#define VIBRATOR_PIN                2

//================================================================
// INPUT PINS
#define BATTERY_VOLTAGE_PIN         A7
#define Monitor_PIN                 7 
#define FAN_RPM_PIN                 3
#define FAN_UP_PIN                  1
#define FAN_DOWN_PIN                9

//================================================================

#define DELAY_10ms 10
#define DELAY_100ms 100
#define DELAY_1SEC 1000
#define DELAY_5SEC 5000

bool Fan_Up_Value;
bool Fan_Down_Value;
bool Monitor_PIN_Value;

uint8_t s_fan_pwm_off = 0;
uint8_t s_fan_pwm_low = 51;
uint8_t s_fan_pwm_mid_low = 102;
uint8_t s_fan_pwm_mid = 153;
uint8_t s_fan_pwm_mid_hi = 204;
uint8_t s_fan_pwm_hi = 255;

int iSpeed = 0;

uint8_t speeds[] = {
    s_fan_pwm_off,
    s_fan_pwm_low,
    s_fan_pwm_mid_low,
    s_fan_pwm_mid,
    s_fan_pwm_mid_hi,
    s_fan_pwm_hi,
};
const int cSpeeds = sizeof(speeds) / sizeof(uint8_t);

//================================================================
void setup() {
    // SETUP INPUT/OUTPUT PINS and Pullup resistor
    DDRB = DDRB | B01000101;
    DDRC = DDRC | B00111111;
    DDRD = DDRD | B10000101;
    PORTD = B10000000;

    //================================================================

      // turn the fan off
    analogWrite(FAN_PWM_PIN, 0);

    // Setup the LED pins as outputs and turn the LEDs off
    PORTC = 0x3f;
    PORTD = 0x81;

    // Buzzer 
    analogWrite(BUZZER_PIN, 0);

    // Vibrator
    digitalWrite(VIBRATOR_PIN, LOW);

    // Flash LEDs on Start

    analogWrite(FAN_PWM_PIN, s_fan_pwm_hi);
}


void loop() {
    digitalWrite(BATTERY_LED_1_PIN, LOW);
    digitalWrite(BATTERY_LED_2_PIN, LOW);
    digitalWrite(BATTERY_LED_3_PIN, LOW);
    digitalWrite(Error_LED_PIN, LOW);
    digitalWrite(MODE_LED_1_PIN, LOW);
    digitalWrite(MODE_LED_2_PIN, LOW);
    digitalWrite(MODE_LED_3_PIN, LOW);
    delay(DELAY_100ms);
    digitalWrite(BATTERY_LED_1_PIN, HIGH);
    digitalWrite(BATTERY_LED_2_PIN, HIGH);
    digitalWrite(BATTERY_LED_3_PIN, HIGH);
    digitalWrite(Error_LED_PIN, HIGH);
    digitalWrite(MODE_LED_1_PIN, HIGH);
    digitalWrite(MODE_LED_2_PIN, HIGH);
    digitalWrite(MODE_LED_3_PIN, HIGH);
    delay(DELAY_100ms);
}
