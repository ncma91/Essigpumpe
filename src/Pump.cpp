#include "Pump.h"

bool Pump::begin(uint8_t pin, uint32_t freqHz, uint8_t resBits) {
    _pin = pin;
    _maxRaw = (1UL << resBits) - 1;

    // Pin sofort auf LOW, damit der Transistor nicht kurz durchschaltet
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);

    // Arduino-ESP32-Core 3.x API
    if (!ledcAttach(_pin, freqHz, resBits)) {
        return false;
    }
    writeRaw(0);
    return true;
}

void Pump::configure(uint8_t dutyPercent, uint32_t onMs, uint32_t offMs, uint32_t kickMs) {
    _dutyPercent = dutyPercent > 100 ? 100 : dutyPercent;
    _onMs = onMs;
    _offMs = offMs;
    _kickMs = kickMs;
}

void Pump::start(bool startOn, uint32_t now) {
    if (disabled()) {
        enterOff(now);
        return;
    }
    if (startOn || continuous()) {
        enterOn(now);
    } else {
        enterOff(now);
    }
}

void Pump::update(uint32_t now) {
    if (disabled()) {
        if (_phase != Phase::Off) enterOff(now);
        return;
    }

    const uint32_t elapsed = now - _phaseStart;   // überlaufsicher

    switch (_phase) {
    case Phase::Kick:
    case Phase::Run:
        if (!continuous() && elapsed >= _onMs) {
            enterOff(now);
        } else if (_phase == Phase::Kick && elapsed >= _kickMs) {
            _phase = Phase::Run;
            writeRaw(percentToRaw(_dutyPercent));
        }
        break;

    case Phase::Off:
        if (elapsed >= _offMs) {
            enterOn(now);
        }
        break;
    }
}

uint8_t Pump::dutyPercent() const {
    return (uint8_t)((_raw * 100UL + _maxRaw / 2) / _maxRaw);
}

uint32_t Pump::remainingMs(uint32_t now) const {
    if (disabled() || (isOn() && continuous())) return 0;
    const uint32_t duration = isOn() ? _onMs : _offMs;
    const uint32_t elapsed = now - _phaseStart;
    return elapsed >= duration ? 0 : duration - elapsed;
}

void Pump::enterOn(uint32_t now) {
    _phaseStart = now;
    const uint32_t target = percentToRaw(_dutyPercent);
    if (_kickMs > 0 && target < _maxRaw) {
        // Softstart: Motor läuft bei kleinem Duty sonst nicht an
        _phase = Phase::Kick;
        writeRaw(_maxRaw);
    } else {
        _phase = Phase::Run;
        writeRaw(target);
    }
}

void Pump::enterOff(uint32_t now) {
    _phaseStart = now;
    _phase = Phase::Off;
    writeRaw(0);
}

void Pump::writeRaw(uint32_t raw) {
    _raw = raw;
    ledcWrite(_pin, raw);
}

uint32_t Pump::percentToRaw(uint8_t percent) const {
    return (percent * _maxRaw + 50) / 100;
}
