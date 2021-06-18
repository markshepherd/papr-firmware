#pragma once

#undef SERIAL_ENABLED

void serialInit();
void serialPrintf(const char* __fmt, ...);
char* renderLongLong(long long num);