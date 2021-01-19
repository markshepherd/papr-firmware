//#include "gmock/gmock.h"  // Brings in gMock.
#include "gtest/gtest.h"

#define BTN_CALLBACK void (*callback)(int)

class ButtonDebounce {
public:
    ButtonDebounce(int pin, unsigned long delay);
    MOCK_METHOD(void, PenUp, (), (override));
    MOCK_METHOD(void, PenDown, (), (override));
    MOCK_METHOD(void, Forward, (int distance), (override));
    MOCK_METHOD(void, Turn, (int degrees), (override));
    MOCK_METHOD(void, GoTo, (int x, int y), (override));
    MOCK_METHOD(int, GetX, (), (const, override));
    MOCK_METHOD(int, GetY, (), (const, override));
};

/*
class ButtonDebounce {
public:
    ButtonDebounce(int pin, unsigned long delay);
    bool isTimeToUpdate();
    void update();
    int state();
    void setCallback(BTN_CALLBACK);

private:
    BTN_CALLBACK;
};
*/