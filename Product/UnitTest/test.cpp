#include "pch.h"
#include "../Main.h"
#include "MyButtonDebounce.h"
#include "MyFanController.h"
#include "../Hardware.h"


class MockTurtle {
public:
    ...
        MOCK_METHOD(void, PenUp, (), (override));
    MOCK_METHOD(void, PenDown, (), (override));
    MOCK_METHOD(void, Forward, (int distance), (override));
    MOCK_METHOD(void, Turn, (int degrees), (override));
    MOCK_METHOD(void, GoTo, (int x, int y), (override));
    MOCK_METHOD(int, GetX, (), (const, override));
    MOCK_METHOD(int, GetY, (), (const, override));
};

Main paprMain;

extern Hardware& hw;
extern ButtonDebounce buttonFanUp;
extern ButtonDebounce buttonFanDown;
extern ButtonDebounce buttonPowerOff;
extern FanController fanController;

TEST(TestSuiteName, TestName) {
    paprMain.setup();
    paprMain.loop();

    EXPECT_EQ(1, 1);
    EXPECT_TRUE(true);
}