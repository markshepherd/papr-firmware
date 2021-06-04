#include "Arduino.h"
#include "FanController.h"
#include "MySerial.h"
#include "Hardware.h"

#define hw Hardware::instance

FanController::FanController(byte sensorPin, unsigned int sensorThreshold, byte pwmPin)
{
	_sensorPin = sensorPin;
	_sensorThreshold = sensorThreshold;
	_pwmPin = pwmPin;
	hw.pinMode(pwmPin, OUTPUT);
	_pwmDutyCycle = 100;
}

void FanController::begin()
{
	hw.digitalWrite(_sensorPin, HIGH);
	setDutyCycle(_pwmDutyCycle);
	_attachInterrupt();
}

unsigned int FanController::getRPM() {
	unsigned long elapsed = hw.millis() - _lastMillis;
	if (elapsed > _sensorThreshold)
	{
		noInterrupts();
		float correctionFactor = 1000.0 / elapsed;
		_lastReading = correctionFactor * _halfRevs / 2 * 60;
		_halfRevs = 0;
		_lastMillis = hw.millis();
		interrupts();
	}
	return _lastReading;
}

void FanController::setDutyCycle(byte dutyCycle) {
	_pwmDutyCycle = min((int)dutyCycle, 100);
	hw.analogWrite(_pwmPin, 2.55 * _pwmDutyCycle);
}

byte FanController::getDutyCycle() {
	return _pwmDutyCycle;
}

void FanController::_attachInterrupt() {
	hw.setFanRPMInterruptCallback(this);
}

void FanController::_detachInterrupt() {
	hw.setFanRPMInterruptCallback(0);
}

void FanController::callback() {
	if (hw.digitalRead(_sensorPin) == LOW) {
		_halfRevs++;
	}
}
