#include "Climate.h"

Climate::Climate(uint8_t pin, uint8_t type, uint32_t intervalMs, uint8_t errorLimit)
    : _dht(pin, type), _intervalMs(intervalMs), _errorLimit(errorLimit) {}

void Climate::begin() {
    _dht.begin();
}

bool Climate::update(uint32_t now) {
    // Erste Messung erst nach einem Intervall: der DHT22 braucht nach dem
    // Einschalten ca. 1–2 s, bevor er sinnvolle Werte liefert.
    if (!_started) {
        _started = true;
        _lastRead = now;
        return false;
    }
    if (now - _lastRead < _intervalMs) return false;
    _lastRead = now;

    // force=true: Bibliothek soll wirklich messen und nicht ihren
    // Cache zurückgeben (ihr eigenes 2-s-Limit läuft nicht synchron zu uns).
    const bool ok = _dht.read(true);
    const float t = _dht.readTemperature();   // nutzt die eben gelesenen Daten
    const float h = _dht.readHumidity();

    const bool plausible = ok && !isnan(t) && !isnan(h) &&
                           t > -40.0f && t < 80.0f &&
                           h >= 0.0f && h <= 100.0f;

    if (plausible) {
        _temperature = t;
        _humidity = h;
        _hasValue = true;
        _lastOk = true;
        _errors = 0;
    } else {
        // letzten gültigen Wert behalten
        _lastOk = false;
        if (_errors < 255) _errors++;
    }
    return true;
}
