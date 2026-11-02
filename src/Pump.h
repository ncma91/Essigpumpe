// Pumpensteuerung: PWM über NPN-Transistor, Softstart, Intervallbetrieb.
// Komplett nicht-blockierend – update() regelmäßig aus loop() aufrufen.
#pragma once

#include <Arduino.h>

class Pump {
public:
    enum class Phase : uint8_t { Off, Kick, Run };

    // PWM-Kanal anlegen, Ausgang auf 0. Liefert false, wenn ledcAttach scheitert.
    bool begin(uint8_t pin, uint32_t freqHz, uint8_t resBits);

    // dutyPercent: 0..100 (0 = Pumpe bleibt aus)
    // onMs:        Länge der An-Phase (inkl. Softstart)
    // offMs:       Länge der Aus-Phase, 0 = Dauerbetrieb
    // kickMs:      Dauer des 100-%-Anlaufimpulses
    void configure(uint8_t dutyPercent, uint32_t onMs, uint32_t offMs, uint32_t kickMs);

    // Intervallbetrieb starten (mit An- oder Aus-Phase).
    void start(bool startOn, uint32_t now);

    void update(uint32_t now);

    bool     isOn() const        { return _phase != Phase::Off; }
    Phase    phase() const       { return _phase; }
    bool     continuous() const  { return _offMs == 0; }
    bool     disabled() const    { return _dutyPercent == 0; }
    uint8_t  dutyPercent() const;                 // aktuell ausgegebener Duty in %
    uint8_t  targetDutyPercent() const { return _dutyPercent; }
    uint32_t onMs() const        { return _onMs; }
    uint32_t offMs() const       { return _offMs; }
    uint32_t remainingMs(uint32_t now) const;     // Restzeit der aktuellen Phase

private:
    void enterOn(uint32_t now);
    void enterOff(uint32_t now);
    void writeRaw(uint32_t raw);
    uint32_t percentToRaw(uint8_t percent) const;

    uint8_t  _pin = 0;
    uint32_t _maxRaw = 255;
    uint32_t _raw = 0;

    uint8_t  _dutyPercent = 0;
    uint32_t _onMs = 0;
    uint32_t _offMs = 0;
    uint32_t _kickMs = 0;

    Phase    _phase = Phase::Off;
    uint32_t _phaseStart = 0;
};
