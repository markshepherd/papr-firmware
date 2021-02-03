#include "MySerial.h"
#include "PAPRHwDefs.h"
#include <SoftwareSerial.h>

#define MOSI_PIN 11  /* PB3 */ 
#define MOSO_PIN 12  /* PB4 */ 
const int serialRxPin = MOSO_PIN;
const int serialTxPin = MOSI_PIN;

SoftwareSerial mySerial(serialRxPin, serialTxPin);

void myPrintf(const char* __fmt, ...) {
	va_list args;
	char buffer[100];
	va_start(args, __fmt);
	vsnprintf(buffer, sizeof(buffer), __fmt, args);
	va_end(args);
    mySerial.print(buffer);
}

void initSerial() {
	mySerial.begin(57600);
}
