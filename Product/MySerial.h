#pragma once

#define SERIAL_ENABLED

#ifdef SERIAL_ENABLED
void serialInit();
void serialPrintf(const char* __fmt, ...);
#endif
