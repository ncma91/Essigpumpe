// =============================================================================
// Essig-Belüftung
//
// ESP32-C3 SuperMini: Mini-Luftpumpe (PWM über NPN-Transistor, Intervallbetrieb),
// AM2302 für Temperatur/Luftfeuchte, Ausgabe über die serielle Schnittstelle.
//
// Alle Pins und Parameter: include/config.h
// Die Hauptschleife ist komplett nicht-blockierend (kein delay()).
// =============================================================================
#include <Arduino.h>
#include "config.h"
#include "Pump.h"
#include "Climate.h"

static Pump    pump;
static Climate climate(PIN_DHT, DHT_TYPE, DHT_INTERVAL_MS, DHT_ERROR_LIMIT);

// -----------------------------------------------------------------------------
// Hilfsfunktionen
// -----------------------------------------------------------------------------

enum class TempState : uint8_t { Ok, Low, High, Unknown };

static TempState tempState() {
    if (!climate.hasValue()) return TempState::Unknown;
#if TEMP_WARN_ENABLED
    if (climate.temperature() < TEMP_MIN_C) return TempState::Low;
    if (climate.temperature() > TEMP_MAX_C) return TempState::High;
#endif
    return TempState::Ok;
}

// -----------------------------------------------------------------------------
// Serielle Ausgabe
// -----------------------------------------------------------------------------

static void logMeasurement(uint32_t now) {
#if SERIAL_ENABLED
    Serial.printf("[%8lu] ", (unsigned long)now);
    if (climate.lastReadOk()) {
        Serial.printf("T=%.1f C  RH=%.1f %%", climate.temperature(), climate.humidity());
    } else {
        Serial.printf("AM2302-Lesefehler (%u in Folge)", climate.errorCount());
        if (climate.hasValue()) {
            Serial.printf(", letzter Wert T=%.1f C RH=%.1f %%",
                          climate.temperature(), climate.humidity());
        }
    }
    Serial.printf("  | Pumpe %s %u%%", pump.isOn() ? "AN" : "AUS", pump.dutyPercent());
    if (!pump.disabled() && !(pump.isOn() && pump.continuous())) {
        Serial.printf(", Rest %lu s", (unsigned long)((pump.remainingMs(now) + 999) / 1000));
    }
    if (climate.sensorError()) Serial.print("  !! Sensorfehler");
    switch (tempState()) {
    case TempState::Low:  Serial.print("  !! Temperatur zu niedrig"); break;
    case TempState::High: Serial.print("  !! Temperatur zu hoch"); break;
    default: break;
    }
    Serial.println();
#else
    (void)now;
#endif
}

// -----------------------------------------------------------------------------
// setup / loop
// -----------------------------------------------------------------------------

void setup() {
    // Pumpe als Erstes initialisieren, damit der Ausgang sofort LOW ist
    const bool pwmOk = pump.begin(PIN_PUMP, PUMP_PWM_FREQ_HZ, PUMP_PWM_RES_BITS);

#if SERIAL_ENABLED
    Serial.begin(SERIAL_BAUD);
    // nicht auf den Monitor warten – die Steuerung soll auch ohne PC laufen
#endif

    climate.begin();

    pump.configure(PUMP_DUTY_PERCENT,
                   PUMP_ON_TIME_S * 1000UL,
                   PUMP_OFF_TIME_S * 1000UL,
                   PUMP_KICK_MS);
    pump.start(PUMP_START_ON, millis());

#if SERIAL_ENABLED
    Serial.println();
    Serial.println("Essig-Belueftung gestartet");
    Serial.printf("Pumpe: GPIO %d, %d Hz, %d Bit, Duty %d %%, an %lu s / aus %lu s%s\n",
                  PIN_PUMP, PUMP_PWM_FREQ_HZ, PUMP_PWM_RES_BITS, PUMP_DUTY_PERCENT,
                  (unsigned long)PUMP_ON_TIME_S, (unsigned long)PUMP_OFF_TIME_S,
                  pwmOk ? "" : "  (ledcAttach FEHLGESCHLAGEN)");
    Serial.printf("AM2302: GPIO %d\n", PIN_DHT);
#else
    (void)pwmOk;
#endif
}

void loop() {
    const uint32_t now = millis();

    pump.update(now);

    if (climate.update(now)) {
        logMeasurement(now);
    }
}
