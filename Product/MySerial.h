#pragma once

#define SERIAL_ENABLED

void serialInit();
void serialPrintf(const char* __fmt, ...);
char* renderDouble(double number, char* pBuffer = 0);
