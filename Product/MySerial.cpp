#include "MySerial.h"

#ifdef SERIAL_ENABLED
#include "PAPRHwDefs.h"
#include <SoftwareSerial.h>

#define MOSI_PIN 11  /* PB3 */ 
#define MOSO_PIN 12  /* PB4 */ 
const int serialRxPin = MOSO_PIN;
const int serialTxPin = MOSI_PIN;

SoftwareSerial mySerial(serialRxPin, serialTxPin);

void MySerial::printf(const char* __fmt, ...) {
	va_list args;
	char buffer[200];
	va_start(args, __fmt);
	vsnprintf(buffer, sizeof(buffer), __fmt, args);
	va_end(args);
	mySerial.print(buffer);
	mySerial.print("\r\n");
}

void MySerial::init() {
	mySerial.begin(57600);
}
#endif
