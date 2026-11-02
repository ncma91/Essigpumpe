// =============================================================================
// Essig-Belüftung – zentrale Konfiguration
//
// ALLE Pins und Parameter stehen hier. Der restliche Code enthält keine
// Pin-Nummern oder Zeitkonstanten.
// =============================================================================
#pragma once

#include <Arduino.h>

// -----------------------------------------------------------------------------
// Pinbelegung – ESP32-C3 SuperMini
// -----------------------------------------------------------------------------
// Herausgeführt: GPIO 0–10, 20, 21
// NICHT verwenden: 2 (Strapping), 8 (Strapping + Onboard-LED),
//                  9 (Strapping + BOOT-Taste), 20/21 (UART0)
// (18/19 = USB und 11–17 = Flash sind ohnehin nicht herausgeführt)
#if !defined(CONFIG_IDF_TARGET_ESP32C3)
  #error "Die Firmware ist für den ESP32-C3 (SuperMini) ausgelegt."
#endif

#define PIN_PUMP            4
#define PIN_DHT             5

// -----------------------------------------------------------------------------
// Pumpe (PWM über NPN-Transistor, Low-Side)
// -----------------------------------------------------------------------------
#define PUMP_PWM_FREQ_HZ    1000        // PWM-Frequenz
#define PUMP_PWM_RES_BITS   8           // Auflösung -> Duty 0..255

#define PUMP_DUTY_PERCENT   60          // Duty im Dauerlauf (0–100 %), 0 = Pumpe nie an
#define PUMP_KICK_MS        500UL       // Softstart: so lange 100 % beim Einschalten

#define PUMP_ON_TIME_S      60UL        // An-Zeit pro Intervall  (1 min)
#define PUMP_OFF_TIME_S     300UL       // Aus-Zeit pro Intervall (5 min), 0 = Dauerbetrieb
#define PUMP_START_ON       true        // nach dem Einschalten mit der An-Phase beginnen

// -----------------------------------------------------------------------------
// AM2302 (= DHT22, gleiches Protokoll)
// -----------------------------------------------------------------------------
#define DHT_TYPE            DHT22       // AM2302 wird als DHT22 angesprochen
#define DHT_INTERVAL_MS     2000UL      // nicht schneller als 2 s lesen!
#define DHT_ERROR_LIMIT     3           // ab so vielen Fehlversuchen in Folge: "Sensorfehler"

// -----------------------------------------------------------------------------
// Temperaturwarnung (Essigbakterien mögen es warm)
// -----------------------------------------------------------------------------
#define TEMP_WARN_ENABLED   1
#define TEMP_MIN_C          24.0f
#define TEMP_MAX_C          30.0f

// -----------------------------------------------------------------------------
// Serielle Ausgabe
// -----------------------------------------------------------------------------
#define SERIAL_ENABLED      1           // Messwerte seriell ausgeben
#define SERIAL_BAUD         115200
