#pragma once

enum FANS {
    FANS_Low = 0,
    FANS_Medium = 1,
    FANS_High = 2,
};

class FanState {
public:
    FanState();

    void increase();
    void decrease();

    inline FANS fans() {
        return m_fans;
    }

private:
    FANS m_fans;
};