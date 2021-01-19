#include "MyButtonDebounce.h"

ButtonDebounce::ButtonDebounce(int pin, unsigned long delay)
{
}

bool ButtonDebounce::isTimeToUpdate()
{
    return false;
}

void ButtonDebounce::update()
{
}

int ButtonDebounce::state()
{
    return 0;
}

void ButtonDebounce::setCallback(BTN_CALLBACK)
{
    this->callback = callback;
}