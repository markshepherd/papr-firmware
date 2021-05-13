/*
  Based on FanController.h/cpp by Giorgio Aresu.
*/
#pragma once
#include "Hardware.h"

class FanController : public InterruptCallback
{
public:
	FanController(byte sensorPin, unsigned int sensorThreshold, byte pwmPin = 0);
	void begin();
	unsigned int getRPM();
	void setDutyCycle(byte dutyCycle);
	byte getDutyCycle();
	
private:
	void _attachInterrupt();
	void _detachInterrupt();
	byte _sensorPin;
	unsigned int _sensorThreshold;
	byte _pwmPin;
	byte _pwmDutyCycle;
	unsigned int _lastReading;
	volatile unsigned int _halfRevs;
	unsigned long _lastMillis;

public:
	virtual void callback();
};
