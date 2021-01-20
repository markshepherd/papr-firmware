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
        main.init(&buttonFanUp, &buttonFanDown, &buttonPowerOff, &fanController, &hw);

        EXPECT_CALL(hw, millis()).Times(AnyNumber()).WillRepeatedly(Return(10000));
        EXPECT_CALL(hw, configurePins()).Times(1).RetiresOnSaturation();
        EXPECT_CALL(hw, initializeDevices()).Times(1).RetiresOnSaturation();
        EXPECT_CALL(buttonFanUp, setCallback_()).Times(1).RetiresOnSaturation();
        EXPECT_CALL(buttonFanDown, setCallback_()).Times(1).RetiresOnSaturation();
        EXPECT_CALL(buttonPowerOff, setCallback_()).Times(1).RetiresOnSaturation();
        EXPECT_CALL(fanController, begin()).Times(1).RetiresOnSaturation();
        EXPECT_CALL(fanController, setDutyCycle(0)).Times(1).RetiresOnSaturation();
        EXPECT_CALL(hw, digitalWrite(FAN_LOW_LED_PIN, LED_ON)).Times(1).RetiresOnSaturation();
        EXPECT_CALL(hw, digitalWrite(FAN_MED_LED_PIN, LED_OFF)).Times(1).RetiresOnSaturation();
        EXPECT_CALL(hw, digitalWrite(FAN_HIGH_LED_PIN, LED_OFF)).Times(1).RetiresOnSaturation();
        main.setup();
    }

    ButtonDebounce buttonFanUp;
    ButtonDebounce buttonFanDown;
    ButtonDebounce buttonPowerOff;
    FanController fanController;
    Hardware hw;
    Main main;
};

TEST_F(PAPRMainTest, Loop)
{
    EXPECT_CALL(hw, analogRead(BATTERY_VOLTAGE_PIN)).Times(1).WillRepeatedly(Return(999)).RetiresOnSaturation();
    EXPECT_CALL(buttonFanUp, update()).Times(1).RetiresOnSaturation();
    EXPECT_CALL(buttonFanDown, update()).Times(1).RetiresOnSaturation();
    EXPECT_CALL(buttonPowerOff, update()).Times(1).RetiresOnSaturation();
    EXPECT_CALL(fanController, getSpeed()).Times(1).RetiresOnSaturation();
    main.loop();
}

TEST_F(PAPRMainTest, ButtonUp)
{
    EXPECT_CALL(hw, digitalWrite(_, _)).Times(0);
    EXPECT_CALL(fanController, setDutyCycle(_)).Times(0);
    buttonFanUp._callback(BUTTON_PUSHED);

    EXPECT_CALL(hw, digitalWrite(FAN_LOW_LED_PIN, LED_ON)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(hw, digitalWrite(FAN_MED_LED_PIN, LED_ON)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(hw, digitalWrite(FAN_HIGH_LED_PIN, LED_OFF)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(fanController, setDutyCycle(50)).Times(1).RetiresOnSaturation();
    buttonFanUp._callback(BUTTON_RELEASED);

    // expect no mock calls
    buttonFanUp._callback(BUTTON_PUSHED);

    EXPECT_CALL(hw, digitalWrite(FAN_LOW_LED_PIN, LED_ON)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(hw, digitalWrite(FAN_MED_LED_PIN, LED_ON)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(hw, digitalWrite(FAN_HIGH_LED_PIN, LED_ON)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(fanController, setDutyCycle(100)).Times(1).RetiresOnSaturation();
    buttonFanUp._callback(BUTTON_RELEASED);

    // expect no mock calls
    buttonFanUp._callback(BUTTON_PUSHED);

    EXPECT_CALL(hw, digitalWrite(FAN_LOW_LED_PIN, LED_ON)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(hw, digitalWrite(FAN_MED_LED_PIN, LED_ON)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(hw, digitalWrite(FAN_HIGH_LED_PIN, LED_ON)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(fanController, setDutyCycle(100)).Times(1).RetiresOnSaturation();
    buttonFanUp._callback(BUTTON_RELEASED);
}

TEST_F(PAPRMainTest, ButtonDown)
{
    EXPECT_CALL(hw, digitalWrite(_, _)).Times(AnyNumber());
    EXPECT_CALL(fanController, setDutyCycle(_)).Times(AnyNumber());
    buttonFanUp._callback(BUTTON_PUSHED);
    buttonFanUp._callback(BUTTON_RELEASED);
    buttonFanUp._callback(BUTTON_PUSHED);
    buttonFanUp._callback(BUTTON_RELEASED);

    EXPECT_CALL(hw, digitalWrite(_, _)).Times(0);
    EXPECT_CALL(fanController, setDutyCycle(_)).Times(0);
    buttonFanDown._callback(BUTTON_PUSHED);

    EXPECT_CALL(hw, digitalWrite(FAN_LOW_LED_PIN, LED_ON)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(hw, digitalWrite(FAN_MED_LED_PIN, LED_ON)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(hw, digitalWrite(FAN_HIGH_LED_PIN, LED_OFF)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(fanController, setDutyCycle(50)).Times(1).RetiresOnSaturation();
    buttonFanDown._callback(BUTTON_RELEASED);

    // expect no mock calls
    buttonFanDown._callback(BUTTON_PUSHED);

    EXPECT_CALL(hw, digitalWrite(FAN_LOW_LED_PIN, LED_ON)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(hw, digitalWrite(FAN_MED_LED_PIN, LED_OFF)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(hw, digitalWrite(FAN_HIGH_LED_PIN, LED_OFF)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(fanController, setDutyCycle(0)).Times(1).RetiresOnSaturation();
    buttonFanDown._callback(BUTTON_RELEASED);

    // expect no mock calls
    buttonFanDown._callback(BUTTON_PUSHED);

    EXPECT_CALL(hw, digitalWrite(FAN_LOW_LED_PIN, LED_ON)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(hw, digitalWrite(FAN_MED_LED_PIN, LED_OFF)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(hw, digitalWrite(FAN_HIGH_LED_PIN, LED_OFF)).Times(1).RetiresOnSaturation();
    EXPECT_CALL(fanController, setDutyCycle(0)).Times(1).RetiresOnSaturation();
    buttonFanDown._callback(BUTTON_RELEASED);
}
