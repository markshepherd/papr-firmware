#include "MySerial.h"

#ifdef SERIAL_ENABLED
//#include "stdarg.h"
//#include "stdio.h"
#include "Arduino.h"
// #include "Hardware.h"
// #include <SoftwareSerial.h>

//#define MOSI_PIN 11  /* PB3 */ 
//#define MOSO_PIN 12  /* PB4 */ 
//const int serialRxPin = MOSO_PIN;
//const int serialTxPin = MOSI_PIN;

//SoftwareSerial mySerial(serialRxPin, serialTxPin);

void serialPrintf(const char* __fmt, ...) {
	va_list args;
	char buffer[300];
	va_start(args, __fmt);
	vsnprintf(buffer, sizeof(buffer), __fmt, args);
	va_end(args);
	Serial.println(buffer);
	//mySerial.print("\r\n");
}

void serialInit() {
	//mySerial.begin(57600);
	Serial.begin(57600);
}

char* renderDouble(double number, char* pBuffer)
{
	if (!pBuffer) {
		const int size = 50;
		static char buffer[size];
		pBuffer = buffer;
	}
	bool negative;
	if (negative = number < 0) number = -number;
	long integerPart = (long)number;
	double fraction = number - (double)integerPart;
	char buff[10];
	sprintf(buff, "%d", (int)(fraction * 1000.0) + 1000);
	sprintf(pBuffer, "%s%ld.%s", (negative ? "-" : ""), integerPart, &buff[1]);
	return pBuffer;
}
#else
#include "Arduino.h"
void serialInit() { Serial.begin(57600); }
void serialPrintf(const char* __fmt, ...) {}
char* renderDouble(double number, char* pBuffer = 0) { return 0; }
#endif
