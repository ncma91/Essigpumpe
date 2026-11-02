// =============================================================================
// Essig-Belüftung
//
// ESP32-C6 / -C3: Mini-Luftpumpe (PWM über NPN-Transistor, Intervallbetrieb),
// DHT22 für Temperatur/Luftfeuchte, Anzeige auf HD44780-LCD.
//
// Alle Pins und Parameter: include/config.h
// Die Hauptschleife ist komplett nicht-blockierend (kein delay()).
// =============================================================================
#include <Arduino.h>
#include "config.h"
#include "Pump.h"
#include "Climate.h"
#include "Display.h"

static Pump    pump;
static Climate climate(PIN_DHT, DHT_TYPE, DHT_INTERVAL_MS, DHT_ERROR_LIMIT);
static Display display;

static uint32_t lastLcdUpdate = 0;

// -----------------------------------------------------------------------------
// Hilfsfunktionen
// -----------------------------------------------------------------------------

// Restzeit als "m:ss" (bis 99:59) bzw. "XhYY" darüber
static void formatDuration(char *out, size_t len, uint32_t ms) {
    const uint32_t s = (ms + 999) / 1000;   // aufrunden: 0:00 erst ganz am Ende
    if (s < 100UL * 60UL) {
        snprintf(out, len, "%lu:%02lu", (unsigned long)(s / 60), (unsigned long)(s % 60));
    } else {
        snprintf(out, len, "%luh%02lu", (unsigned long)(s / 3600), (unsigned long)((s / 60) % 60));
    }
}

enum class TempState : uint8_t { Ok, Low, High, Unknown };

static TempState tempState() {
    if (!climate.hasValue()) return TempState::Unknown;
#if TEMP_WARN_ENABLED
    if (climate.temperature() < TEMP_MIN_C) return TempState::Low;
    if (climate.temperature() > TEMP_MAX_C) return TempState::High;
#endif
    return TempState::Ok;
}

// Liste aktiver Warnungen (max. 2), Rückgabe = Anzahl
static uint8_t collectWarnings(const char *warn[2]) {
    uint8_t n = 0;
    if (climate.sensorError()) warn[n++] = "DHT22 Fehler!";
    switch (tempState()) {
    case TempState::Low:  warn[n++] = "WARNUNG zu kalt"; break;
    case TempState::High: warn[n++] = "WARNUNG zu warm"; break;
    default: break;
    }
    return n;
}

// -----------------------------------------------------------------------------
// Anzeige
// -----------------------------------------------------------------------------

static void showClimate(uint8_t row) {
    char left[12], right[12];
    if (climate.hasValue()) {
        snprintf(left, sizeof(left), "%.1f%cC", climate.temperature(), LCD_DEGREE);
        snprintf(right, sizeof(right), "%.0f%% rF", climate.humidity());
    } else {
        snprintf(left, sizeof(left), "--.-%cC", LCD_DEGREE);
        snprintf(right, sizeof(right), "--%% rF");
    }
    display.printLR(row, left, right);
}

static void showPump(uint8_t row, uint32_t now) {
    char state[20], right[12];

    if (pump.disabled()) {
        display.printLR(row, "Pumpe AUS", "Duty 0%");
        return;
    }

    if (pump.isOn()) {
        snprintf(state, sizeof(state), "Pumpe AN %u%%", pump.dutyPercent());
    } else {
        snprintf(state, sizeof(state), "Pumpe AUS");
    }

    if (pump.isOn() && pump.continuous()) {
        snprintf(right, sizeof(right), "Dauer");
    } else {
        formatDuration(right, sizeof(right), pump.remainingMs(now));
    }

    // Wird es zu eng (z. B. "Pumpe AN 100%" + "12:34"), "Pumpe " weglassen
    const char *left = state;
    if (strlen(state) + 1 + strlen(right) > display.cols()) {
        left = state + 6;
    }
    display.printLR(row, left, right);
}

static void updateDisplay(uint32_t now) {
    const char *warn[2];
    const uint8_t nWarn = collectWarnings(warn);
    // Wechselnder Index: 0 = Messwerte, 1..n = Warnungen
    const uint8_t slot = (now / LCD_ALT_MS) % (nWarn + 1);

    if (display.rows() >= 4) {
        // 20x4: Messwerte fest in Zeile 1, Warnungen in Zeile 3, Takt in Zeile 4
        showClimate(0);
        showPump(1, now);

        if (nWarn > 0) {
            display.printLine(2, warn[(now / LCD_ALT_MS) % nWarn]);
        } else if (climate.hasValue()) {
            char buf[32];   // printLine kürzt auf LCD_COLS
            snprintf(buf, sizeof(buf), "Temp. OK (%.0f-%.0f%cC)",
                     (double)TEMP_MIN_C, (double)TEMP_MAX_C, LCD_DEGREE);
            display.printLine(2, buf);
        } else {
            display.printLine(2, "Warte auf DHT22...");
        }

        char on[8], off[8], buf[32];
        formatDuration(on, sizeof(on), pump.onMs());
        if (pump.continuous()) {
            snprintf(buf, sizeof(buf), "Takt: Dauerbetrieb");
        } else {
            formatDuration(off, sizeof(off), pump.offMs());
            snprintf(buf, sizeof(buf), "An %s / Aus %s", on, off);
        }
        display.printLine(3, buf);
    } else {
        // 16x2: Zeile 1 wechselt zwischen Messwerten und Warnungen
        if (slot == 0) {
            showClimate(0);
        } else {
            display.printLine(0, warn[slot - 1]);
        }
        showPump(1, now);
    }
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
        Serial.printf("DHT22-Lesefehler (%u in Folge)", climate.errorCount());
        if (climate.hasValue()) {
            Serial.printf(", letzter Wert T=%.1f C RH=%.1f %%",
                          climate.temperature(), climate.humidity());
        }
    }
    Serial.printf("  | Pumpe %s %u%%", pump.isOn() ? "AN" : "AUS", pump.dutyPercent());
    if (!pump.disabled() && !(pump.isOn() && pump.continuous())) {
        Serial.printf(", Rest %lu s", (unsigned long)((pump.remainingMs(now) + 999) / 1000));
    }
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

    display.begin();
    display.printLine(0, "Essig-Belueftung");
    display.printLine(1, pwmOk ? "Start..." : "PWM-Fehler!");

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
    Serial.printf("DHT22: GPIO %d, LCD: %s %dx%d\n", PIN_DHT,
                  LCD_MODE == LCD_MODE_I2C ? "I2C" : "parallel", LCD_COLS, LCD_ROWS);
#endif

    lastLcdUpdate = millis();
}

void loop() {
    const uint32_t now = millis();

    pump.update(now);

    if (climate.update(now)) {
        logMeasurement(now);
    }

    if (now - lastLcdUpdate >= LCD_REFRESH_MS) {
        lastLcdUpdate = now;
        updateDisplay(now);
    }
}
