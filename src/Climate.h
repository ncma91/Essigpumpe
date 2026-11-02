// AM2302-Auswertung: liest in festem Intervall, hält den letzten gültigen
// Messwert und zählt Fehlversuche.
#pragma once

#include <Arduino.h>
#include <DHT.h>

class Climate {
public:
    Climate(uint8_t pin, uint8_t type, uint32_t intervalMs, uint8_t errorLimit);

    void begin();

    // Liefert true, wenn in diesem Aufruf eine Messung stattgefunden hat.
    bool update(uint32_t now);

    bool  hasValue() const     { return _hasValue; }    // je ein gültiger Wert?
    float temperature() const  { return _temperature; } // letzter gültiger Wert (°C)
    float humidity() const     { return _humidity; }    // letzter gültiger Wert (%rF)
    bool  lastReadOk() const   { return _lastOk; }
    uint8_t errorCount() const { return _errors; }      // Fehler in Folge
    bool  sensorError() const  { return _errors >= _errorLimit; }

private:
    DHT      _dht;
    uint32_t _intervalMs;
    uint8_t  _errorLimit;

    bool     _started = false;
    uint32_t _lastRead = 0;
    bool     _hasValue = false;
    bool     _lastOk = false;
    uint8_t  _errors = 0;
    float    _temperature = NAN;
    float    _humidity = NAN;
};
