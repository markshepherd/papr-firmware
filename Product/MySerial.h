#pragma once

#define SERIAL_ENABLED

#ifdef SERIAL_ENABLED
void serialInit();
void serialPrintf(const char* __fmt, ...);
char* renderDouble(double number, char* pBuffer = 0);
#else
void serialInit();
void serialPrintf(const char* __fmt, ...);
char* renderDouble(double number, char* pBuffer = 0);
#endif
