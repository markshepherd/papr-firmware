#include <stdint.h>
typedef uint8_t byte;

class FanController {
public:
    FanController::FanController(byte sensorPin, unsigned int sensorThreshold, byte pwmPin);
    void FanController::begin();
    unsigned int FanController::getSpeed();
    void FanController::setDutyCycle(byte dutyCycle);
    byte FanController::getDutyCycle();
};