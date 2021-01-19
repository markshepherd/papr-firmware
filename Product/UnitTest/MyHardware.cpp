#include "../Hardware.h"


void Hardware::pinMode(uint8_t pin, uint8_t mode)
{
}

void Hardware::digitalWrite(uint8_t pin, uint8_t val)
{
}

int Hardware::digitalRead(uint8_t pin)
{
    return 0;
}

int Hardware::analogRead(uint8_t pin)
{
    return 0;
}

void Hardware::analogWrite(uint8_t pin, int val)
{
}

unsigned long Hardware::millis(void)
{
    return 0;
}

void Hardware::delay(unsigned long ms)
{
}

void Hardware::delayMicroseconds(unsigned int us)
{
}

/********************************************************************
 * PAPR-specific functions
 ********************************************************************/

 // Configure all the microcontroller IO pins that this app uses.
void Hardware::configurePins()
{
}

// Set all devices to an initial state
void Hardware::initializeDevices()
{
}


Hardware Hardware::hardwareInstance;

Hardware& Hardware::instance()
{
    return hardwareInstance;
}