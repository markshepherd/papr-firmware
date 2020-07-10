#include "FanState.h"

FanState::FanState():
    m_fans(FANS_Low) {}

void FanState::increase() {
    switch (m_fans) {
    case FANS_Low:
        m_fans = FANS_Medium;
        break;
    case FANS_Medium:
        m_fans = FANS_High;
        break;
    }
}

void FanState::decrease() {
    switch (m_fans) {
    case FANS_Medium:
        m_fans = FANS_Low;
        break;
    case FANS_High:
        m_fans = FANS_Medium;
        break;
    }
}