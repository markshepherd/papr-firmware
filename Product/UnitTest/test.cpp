#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "../PAPRHwDefs.h"
#include "../Main.h"

#include "MyButtonDebounce.h"
#include "MyFanController.h"
#include "MyHardware.h"

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Return;

class PAPRMainTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // Check the data that is set up by the constructor
        EXPECT_EQ(m.buttonFanDown._pin, FAN_DOWN_PIN);
        EXPECT_EQ(m.buttonFanUp._pin, FAN_UP_PIN);
        EXPECT_EQ(m.buttonPowerOff._pin, MONITOR_PIN);
        EXPECT_EQ(m.buttonFanDown._delay, 100);
        EXPECT_EQ(m.buttonFanUp._delay, 100);
        EXPECT_EQ(m.buttonPowerOff._delay, 100);
        EXPECT_EQ(m.fanController._sensorPin, FAN_RPM_PIN);
        EXPECT_EQ(m.fanController._sensorThreshold, 1000);
        EXPECT_EQ(m.fanController._pwmPin, FAN_PWM_PIN);

        // Check the results of the setup() function.
        EXPECT_CALL(m.hw, millis()).Times(AnyNumber()).WillRepeatedly(Return(10000));
        EXPECT_CALL(m.hw, configurePins()).Times(1).RetiresOnSaturation();
        EXPECT_CALL(m.hw, initializeDevices()).Times(1).RetiresOnSaturation();
        EXPECT_CALL(m.buttonFanUp, setCallback_()).Times(1).RetiresOnSaturation();
        EXPECT_CALL(m.buttonFanDown, setCallback_()).Times(1).RetiresOnSaturation();
        EXPECT_CALL(m.buttonPowerOff, setCallback_()).Times(1).RetiresOnSaturation();
        EXPECT_CALL(m.fanController, begin()).Times(1).RetiresOnSaturation();
        EXPECT_CALL(m.fanController, setDutyCycle(0)).Times(1).RetiresOnSaturation();
        EXPECT_CALL(m.hw, digitalWrite(FAN_LOW_LED_PIN, LED_ON)).Times(1).RetiresOnSaturation();
        EXPECT_CALL(m.hw, digitalWrite(FAN_MED_LED_PIN, LED_OFF)).Times(1).RetiresOnSaturation();
        EXPECT_CALL(m.hw, digitalWrite(FAN_HIGH_LED_PIN, LED_OFF)).Times(1).RetiresOnSaturation();
        m.setup();
    }

    Main m;
};

TEST_F(PAPRMainTest, Loop)
{
    // Check the basic operation of the main loop
    EXPECT_CALL(m.hw, analogRead(BATTERY_VOLTAGE_PIN)).Times(1).WillRepeatedly(Return(999)).RetiresOnSaturation();
    EXPECT_CALL(m.buttonFanUp, update()).Times(1).RetiresOnSaturation();
    EXPECT_CALL(m.buttonFanDown, update()).Times(1).RetiresOnSaturation();
    EXPECT_CALL(m.buttonPowerOff, update()).Times(1).RetiresOnSaturation();
    EXPECT_CALL(m.fanController, getSpeed()).Times(1).RetiresOnSaturation();
    m.loop();
}

TEST_F(PAPRMainTest, ButtonUp)
{
    // Push Up button, expect nothing happens
    EXPECT_CALL(m.hw, digitalWrite(_, _)).Times(0);
    EXPECT_CALL(m.fanController, setDutyCycle(_)).Times(0);
    m.buttonFanUp._callback(BUTTON_PUSHED);

    // Release Up button, expect LEDs and fan speed change to medium
    EXPECT_CALL(m.hw, digitalWrite(FAN_LOW_LED_PIN, LED_ON)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(m.hw, digitalWrite(FAN_MED_LED_PIN, LED_ON)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(m.hw, digitalWrite(FAN_HIGH_LED_PIN, LED_OFF)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(m.fanController, setDutyCycle(50)).Times(1).RetiresOnSaturation();
    m.buttonFanUp._callback(BUTTON_RELEASED);

    // Push Up button again, expect nothing happens.
    m.buttonFanUp._callback(BUTTON_PUSHED);

    // Release Up button, expect LEDs and fan speed change to high
    EXPECT_CALL(m.hw, digitalWrite(FAN_LOW_LED_PIN, LED_ON)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(m.hw, digitalWrite(FAN_MED_LED_PIN, LED_ON)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(m.hw, digitalWrite(FAN_HIGH_LED_PIN, LED_ON)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(m.fanController, setDutyCycle(100)).Times(1).RetiresOnSaturation();
    m.buttonFanUp._callback(BUTTON_RELEASED);

    // Push Up button again, expect nothing happens.
    m.buttonFanUp._callback(BUTTON_PUSHED);

    // Release Up button, LEDs and fan speed remain at high.
    EXPECT_CALL(m.hw, digitalWrite(FAN_LOW_LED_PIN, LED_ON)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(m.hw, digitalWrite(FAN_MED_LED_PIN, LED_ON)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(m.hw, digitalWrite(FAN_HIGH_LED_PIN, LED_ON)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(m.fanController, setDutyCycle(100)).Times(1).RetiresOnSaturation();
    m.buttonFanUp._callback(BUTTON_RELEASED);
}

TEST_F(PAPRMainTest, ButtonDown)
{
    // Push and release the UP button twice, to set the fan speed to high.
    EXPECT_CALL(m.hw, digitalWrite(_, _)).Times(AnyNumber());
    EXPECT_CALL(m.fanController, setDutyCycle(_)).Times(AnyNumber());
    m.buttonFanUp._callback(BUTTON_PUSHED);
    m.buttonFanUp._callback(BUTTON_RELEASED);
    m.buttonFanUp._callback(BUTTON_PUSHED);
    m.buttonFanUp._callback(BUTTON_RELEASED);

    // Push Down button, expect nothing happens
    EXPECT_CALL(m.hw, digitalWrite(_, _)).Times(0);
    EXPECT_CALL(m.fanController, setDutyCycle(_)).Times(0);
    m.buttonFanDown._callback(BUTTON_PUSHED);

    // Release Down button, expect LEDs and fan speed change to medium
    EXPECT_CALL(m.hw, digitalWrite(FAN_LOW_LED_PIN, LED_ON)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(m.hw, digitalWrite(FAN_MED_LED_PIN, LED_ON)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(m.hw, digitalWrite(FAN_HIGH_LED_PIN, LED_OFF)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(m.fanController, setDutyCycle(50)).Times(1).RetiresOnSaturation();
    m.buttonFanDown._callback(BUTTON_RELEASED);

    // Push Down button, expect nothing happens
    m.buttonFanDown._callback(BUTTON_PUSHED);

    // Release Down buttonm, expect LEDs and fan speed change to low
    EXPECT_CALL(m.hw, digitalWrite(FAN_LOW_LED_PIN, LED_ON)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(m.hw, digitalWrite(FAN_MED_LED_PIN, LED_OFF)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(m.hw, digitalWrite(FAN_HIGH_LED_PIN, LED_OFF)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(m.fanController, setDutyCycle(0)).Times(1).RetiresOnSaturation();
    m.buttonFanDown._callback(BUTTON_RELEASED);

    // Push Down button, expect nothing happens
    m.buttonFanDown._callback(BUTTON_PUSHED);

    // Release button, expect LEDs and fan speed remain at low.
    EXPECT_CALL(m.hw, digitalWrite(FAN_LOW_LED_PIN, LED_ON)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(m.hw, digitalWrite(FAN_MED_LED_PIN, LED_OFF)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(m.hw, digitalWrite(FAN_HIGH_LED_PIN, LED_OFF)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(m.fanController, setDutyCycle(0)).Times(1).RetiresOnSaturation();
    m.buttonFanDown._callback(BUTTON_RELEASED);
}

TEST_F(PAPRMainTest, ButtonPowerOff)
{
    // When you push the power off button, it should turn on all the LEDs and buzzer. 
    // The fan speed should not change. The code should then repeatedly call update() and
    // check the button state. When the button state becomes Released, it should 
    // turn off all LEDs and buzzer, then restore the current fan settings.
    EXPECT_CALL(m.hw, digitalWrite(_, LED_ON)).Times(7).RetiresOnSaturation();
    EXPECT_CALL(m.hw, analogWrite(BUZZER_PIN, BUZZER_ON)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(m.fanController, setDutyCycle(_)).Times(0);
    EXPECT_CALL(m.buttonPowerOff, state()).Times(1).WillRepeatedly(Return(BUTTON_RELEASED)).RetiresOnSaturation();
    EXPECT_CALL(m.buttonPowerOff, state()).Times(5).WillRepeatedly(Return(BUTTON_PUSHED)).RetiresOnSaturation();
    EXPECT_CALL(m.buttonPowerOff, update()).Times(5).RetiresOnSaturation();
    EXPECT_CALL(m.hw, digitalWrite(_, LED_OFF)).Times(7).RetiresOnSaturation();
    EXPECT_CALL(m.hw, analogWrite(BUZZER_PIN, BUZZER_OFF)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(m.hw, digitalWrite(FAN_LOW_LED_PIN, LED_ON)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(m.hw, digitalWrite(FAN_MED_LED_PIN, LED_OFF)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(m.hw, digitalWrite(FAN_HIGH_LED_PIN, LED_OFF)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(m.fanController, setDutyCycle(0)).Times(1).RetiresOnSaturation();
    m.buttonPowerOff._callback(BUTTON_PUSHED);
}