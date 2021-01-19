#include "../Libraries/FanController/1.0.6/FanController/FanController.h"

FanController::FanController(byte sensorPin, unsigned int sensorThreshold, byte pwmPin)
{
}

void FanController::begin()
{
}

unsigned int FanController::getSpeed()
{
	return 0;
}

void FanController::setDutyCycle(byte dutyCycle)
{
}

byte FanController::getDutyCycle()
{
	return 0;
}

void FanController::_attachInterrupt()
{
}

FanController* FanController::_instances[6];

void FanController::_trigger()
{
}

void FanController::_triggerCaller(byte instance)
{
}
