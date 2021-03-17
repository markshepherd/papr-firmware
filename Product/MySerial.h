#pragma once

#define SERIAL_ENABLED

class MySerial {
public:
#ifdef SERIAL_ENABLED
    static void init();
    static void printf(const char* __fmt, ...);
#else
    static void init() {}
    static void printf(const char* __fmt, ...) {}
#endif
};