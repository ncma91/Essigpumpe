// =============================================================================
// Essig-Belüftung – zentrale Konfiguration
//
// ALLE Pins und Parameter stehen hier. Der restliche Code enthält keine
// Pin-Nummern oder Zeitkonstanten.
// =============================================================================
#pragma once

#include <Arduino.h>

// -----------------------------------------------------------------------------
// Display-Anschluss: genau EINE Variante aktivieren
// -----------------------------------------------------------------------------
#define LCD_MODE_I2C        1   // HD44780 mit PCF8574-I²C-Backpack
#define LCD_MODE_PARALLEL   2   // HD44780 direkt im 4-Bit-Modus, R/W fest auf GND

// (alle LCD_*-Werte lassen sich auch per build_flags in platformio.ini
//  überschreiben, z. B. -D LCD_MODE=LCD_MODE_PARALLEL)
#ifndef LCD_MODE
#define LCD_MODE            LCD_MODE_I2C
#endif

// Displaygröße (16x2 oder 20x4)
#ifndef LCD_COLS
#define LCD_COLS            16
#endif
#ifndef LCD_ROWS
#define LCD_ROWS            2
#endif

// I²C-Adresse des Backpacks: PCF8574 meist 0x27, PCF8574A meist 0x3F
#ifndef LCD_I2C_ADDR
#define LCD_I2C_ADDR        0x27
#endif

// -----------------------------------------------------------------------------
// Pinbelegung – abhängig vom Chip (wird automatisch erkannt)
// -----------------------------------------------------------------------------
#if defined(CONFIG_IDF_TARGET_ESP32C6)
// ESP32-C6 (nanoESP32-C6, ESP32-C6-WROOM-1)
// NICHT verwenden: 4, 5, 8, 9, 15 (Strapping), 12/13 (USB),
//                  16/17 (UART0), 24–30 (Flash)
// GPIO 0/1 sind die XTAL_32K-Pins – frei nutzbar, solange kein 32-kHz-Quarz
// bestückt ist.
  #define PIN_PUMP          18
  #define PIN_DHT           19
  #define PIN_I2C_SDA       6
  #define PIN_I2C_SCL       7
  #define PIN_LCD_RS        0
  #define PIN_LCD_EN        1
  #define PIN_LCD_D4        2
  #define PIN_LCD_D5        3
  #define PIN_LCD_D6        10
  #define PIN_LCD_D7        11

#elif defined(CONFIG_IDF_TARGET_ESP32C3)
// ESP32-C3
// NICHT verwenden: 2, 8, 9 (Strapping), 18/19 (USB), 11–17 (Flash),
//                  20/21 (UART0, nur wenn Serial über USB läuft frei)
// Paralleler Modus braucht 8 Pins; I²C- und Parallel-Pins dürfen sich
// überschneiden, da immer nur eine Variante aktiv ist.
  #define PIN_PUMP          4
  #define PIN_DHT           5
  #define PIN_I2C_SDA       6
  #define PIN_I2C_SCL       7
  #define PIN_LCD_RS        0
  #define PIN_LCD_EN        1
  #define PIN_LCD_D4        3
  #define PIN_LCD_D5        6
  #define PIN_LCD_D6        7
  #define PIN_LCD_D7        10

#else
  #error "Unbekannter Chip – Pinbelegung in include/config.h ergänzen."
#endif

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
// DHT22
// -----------------------------------------------------------------------------
#define DHT_TYPE            DHT22
#define DHT_INTERVAL_MS     2000UL      // nicht schneller als 2 s lesen!
#define DHT_ERROR_LIMIT     3           // ab so vielen Fehlversuchen in Folge: "Sensorfehler"

// -----------------------------------------------------------------------------
// Temperaturwarnung (Essigbakterien mögen es warm)
// -----------------------------------------------------------------------------
#define TEMP_WARN_ENABLED   1
#define TEMP_MIN_C          24.0f
#define TEMP_MAX_C          30.0f

// -----------------------------------------------------------------------------
// Anzeige / Debug
// -----------------------------------------------------------------------------
#define LCD_REFRESH_MS      250UL       // Displayaktualisierung
#define LCD_ALT_MS          2000UL      // Wechselintervall Messwerte <-> Warnung
#define SERIAL_ENABLED      1           // Messwerte seriell ausgeben
#define SERIAL_BAUD         115200
