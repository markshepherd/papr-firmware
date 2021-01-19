#include "../Libraries/ButtonDebounce/1.0.1/ButtonDebounce/src/ButtonDebounce.h"

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