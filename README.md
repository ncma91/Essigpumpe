# Essig-Belüftung

Kleine Steuerung für die Essigherstellung: Eine Mini-Luftpumpe belüftet ein Glas mit Essigansatz, ein DHT22 misst Temperatur und Luftfeuchte, die Werte stehen auf einem Zeichen-LCD. Ein ESP32-C6 (oder -C3) schaltet die Pumpe über einen NPN-Transistor (kein Relais), mit PWM-Drosselung, Softstart und Intervallbetrieb.

## Funktionen

- **Temperatur/Luftfeuchte**: DHT22 wird alle 2 s gelesen. Bei Lesefehlern bleibt der letzte gültige Wert stehen; nach 3 Fehlversuchen in Folge erscheint „DHT22 Fehler!“.
- **LCD**: Zeile 1 zeigt Temperatur und Luftfeuchte, Zeile 2 den Pumpenstatus (AN/AUS, Duty in %, Restzeit der Phase). Bei 20×4 kommen Warnungen und der eingestellte Takt in die Zeilen 3 und 4.
- **PWM-Pumpe**: 1 kHz, 8 Bit, Arduino-ESP32-Core-3.x-API (`ledcAttach` / `ledcWrite`).
- **Softstart**: Beim Einschalten 500 ms lang 100 % Duty, danach der eingestellte Wert, damit der Motor auch bei niedrigem Duty anläuft.
- **Intervallbetrieb**: standardmäßig 1 min an und 5 min aus bei 60 % Duty. Läuft nicht-blockierend mit `millis()`, ohne `delay()` in der Schleife.
- **Temperaturwarnung**: Liegt die Temperatur außerhalb von 24–30 °C, wechselt die Anzeige im 2-s-Takt zwischen Messwert und Warnung.
- **Serielle Ausgabe**: Messwerte und Pumpenstatus mit 115200 Baud.

Anzeige 16×2 (Beispiel):

```
26.4°C    71% rF
AN 60%      0:42
```

## Hardware / Stückliste

| Anz. | Bauteil | Bemerkung |
|---|---|---|
| 1 | nanoESP32-C6 (ESP32-C6-WROOM-1, 2× USB-C) | **Vor dem Build prüfen: C6 oder C3?** Die Firmware kann beides. |
| 1 | Netzteil 230 V AC → 5 V DC, 3 A | versorgt ESP32 (5V-Pin), Pumpe und LCD |
| 1 | 370er Mini-Luftpumpe, 3–9 V DC | an 5 V betrieben; ca. 200–400 mA, Anlauf kurz bis ~1 A (**nachmessen**) |
| 1 | NPN-Transistor BC337 (800 mA, 1 A Spitze) | Alternativen: 2N2222, S8050 (anderes Pinout!) |
| 1 | Widerstand 220 Ω | Basiswiderstand; bei Pumpenstrom > 400 mA: 150 Ω |
| 1 | Widerstand 10 kΩ | Pull-down Basis → GND (empfohlen, hält die Pumpe beim Booten aus) |
| 1 | Diode 1N4007 oder 1N5819 | Freilaufdiode, **Pflicht** |
| 1 | Elko 470–1000 µF, ≥ 10 V | Puffer an 5 V nahe der Pumpe (gegen Brown-out-Resets) |
| 1 | DHT22 (AM2302) | an 3,3 V |
| 1 | Widerstand 10 kΩ | Pull-up an der DHT22-Datenleitung (bei Modulen oft schon drauf) |
| 1 | Zeichen-LCD HD44780-kompatibel, 16×2 oder 20×4, 5 V | z. B. WD-S1103L-7ELNA (Datenblatt nicht gefunden, HD44780 angenommen) |
| (1) | I²C-Backpack PCF8574 | nur für die I²C-Variante |
| (1) | Trimmer 10 kΩ | nur für die Parallel-Variante (Kontrast V0) |
| – | Schlauch, Ausströmerstein, Luftfilter | Watte oder Spritzenfilter am Pumpeneingang gegen Fremdkeime und Essigfliegen |

## Schaltung

### Stromversorgung

```
Netzteil +5V ──┬── ESP32 5V-Pin
               ├── LCD VCC (5 V)
               ├── Pumpe (+)
               └── Elko (+) 470–1000 µF ── GND

Netzteil GND ──── gemeinsame Masse: ESP32, Transistor-Emitter, LCD, DHT22, Elko (–)
```

**Alle GNDs gemeinsam verbinden.** Die Pumpe nicht aus dem 3,3-V-Regler des Boards versorgen.

### Pumpe (Low-Side-Schalter)

```
5V ──┬──────────────┐
     │              │
  Pumpe (+)     Diode 1N4007 / 1N5819
     │          (Kathode = Ring an 5V)
  Pumpe (–) ────────┘
     │
  Kollektor BC337
  Emitter ── GND

GPIO (Pumpe) ── 220 Ω ──┬── Basis BC337
                        │
                      10 kΩ (optional, empfohlen)
                        │
                       GND
```

- **BC337-Pinout** (flache Seite zum Betrachter, Beine unten): **C – B – E**
- **S8050 und 2N2222 (TO-92)**: umgekehrt, **E – B – C**
- Basisstrom mit 220 Ω: (3,3 V − 0,8 V) / 220 Ω ≈ 11 mA. Das reicht für ~400 mA im Sättigungsbetrieb. Bei höherem Pumpenstrom 150 Ω (≈ 17 mA) verwenden. Der ESP32-GPIO verträgt das.
- Die Freilaufdiode schützt den Transistor vor der Induktionsspannung des Motors, vor allem bei PWM.
- Der Elko fängt den Anlaufstrom ab. Ohne ihn kann der ESP32 beim Einschalten der Pumpe resetten (Brown-out).

### DHT22

```
DHT22 Pin 1 (VCC)  ── 3,3 V
DHT22 Pin 2 (Data) ── GPIO (DHT) ── 10 kΩ ── 3,3 V
DHT22 Pin 3        ── nicht belegt
DHT22 Pin 4 (GND)  ── GND
```

### LCD – Variante A: I²C-Backpack (Standard)

```
Backpack VCC ── 5 V
Backpack GND ── GND
Backpack SDA ── GPIO (SDA)
Backpack SCL ── GPIO (SCL)
```

> **Achtung, Pegel:** Viele PCF8574-Backpacks haben 4,7-kΩ-Pull-ups von SDA/SCL nach **5 V**. Die ESP32-Pins sind nicht 5-V-tolerant. Entweder die Pull-ups auf dem Backpack entfernen (der ESP32 bzw. externe 4,7 kΩ nach 3,3 V übernehmen dann) oder einen I²C-Pegelwandler einsetzen. Der Kontrast wird am blauen Trimmer auf dem Backpack eingestellt.

I²C-Adresse: PCF8574 meist `0x27`, PCF8574A meist `0x3F` (in `config.h` → `LCD_I2C_ADDR`).

### LCD – Variante B: parallel, 4-Bit-Modus

| LCD-Pin | Signal | Anschluss |
|---|---|---|
| 1 | VSS | GND |
| 2 | VDD | 5 V |
| 3 | V0 | Schleifer 10-kΩ-Trimmer (zwischen 5 V und GND) |
| 4 | RS | GPIO (RS) |
| 5 | R/W | **fest an GND** (es wird nur geschrieben, das LCD treibt die Leitungen nie, kein 5-V-Problem) |
| 6 | E | GPIO (EN) |
| 7–10 | D0–D3 | offen |
| 11–14 | D4–D7 | GPIO (D4–D7) |
| 15 | A (LED +) | 5 V, ggf. über Vorwiderstand (siehe Modul) |
| 16 | K (LED −) | GND |

Ein 5-V-HD44780 erkennt die 3,3-V-Pegel des ESP32 in der Praxis zuverlässig (V_IH ≈ 2,2 V).

## Pinbelegung

Alle Pins stehen in [`include/config.h`](include/config.h). Der Chip wird beim Kompilieren automatisch erkannt.

| Funktion | ESP32-C6 | ESP32-C3 |
|---|---|---|
| Pumpe (PWM) | GPIO 18 | GPIO 4 |
| DHT22 Data | GPIO 19 | GPIO 5 |
| I²C SDA (LCD-Backpack) | GPIO 6 | GPIO 6 |
| I²C SCL (LCD-Backpack) | GPIO 7 | GPIO 7 |
| LCD parallel RS / EN | GPIO 0 / 1 | GPIO 0 / 1 |
| LCD parallel D4 / D5 / D6 / D7 | GPIO 2 / 3 / 10 / 11 | GPIO 3 / 6 / 7 / 10 |

Pins, die frei bleiben müssen:

- **ESP32-C6**: 4, 5, 8, 9, 15 (Strapping), 12/13 (USB), 16/17 (UART0), 24–30 (Flash). GPIO 0/1 sind die 32-kHz-Quarz-Pins und nur frei, wenn kein Quarz bestückt ist.
- **ESP32-C3**: 2, 8, 9 (Strapping), 18/19 (USB), 11–17 (Flash). Beim C3 reichen die freien Pins für den Parallelmodus nur knapp, deshalb teilen sich dort LCD-D5/D6 die Pins mit SDA/SCL (es ist immer nur eine LCD-Variante aktiv).

## Konfiguration

Alles steht in `include/config.h`:

| Parameter | Default | Bedeutung |
|---|---|---|
| `LCD_MODE` | `LCD_MODE_I2C` | `LCD_MODE_I2C` oder `LCD_MODE_PARALLEL` |
| `LCD_COLS` / `LCD_ROWS` | 16 / 2 | Displaygröße (16×2 oder 20×4) |
| `LCD_I2C_ADDR` | `0x27` | Adresse des Backpacks |
| `PUMP_DUTY_PERCENT` | 60 | Pumpenleistung in % (0 = Pumpe aus) |
| `PUMP_ON_TIME_S` | 60 | An-Zeit in s |
| `PUMP_OFF_TIME_S` | 300 | Aus-Zeit in s (0 = Dauerbetrieb) |
| `PUMP_KICK_MS` | 500 | Softstart-Dauer mit 100 % |
| `PUMP_START_ON` | `true` | nach dem Einschalten mit der An-Phase beginnen |
| `PUMP_PWM_FREQ_HZ` / `PUMP_PWM_RES_BITS` | 1000 / 8 | PWM-Parameter |
| `DHT_INTERVAL_MS` | 2000 | Messintervall (nicht unter 2000) |
| `TEMP_MIN_C` / `TEMP_MAX_C` | 24 / 30 | Warnbereich |
| `TEMP_WARN_ENABLED` | 1 | Temperaturwarnung an/aus |
| `SERIAL_ENABLED` | 1 | serielle Ausgabe an/aus |

Die LCD-Werte lassen sich auch in `platformio.ini` überschreiben, ohne `config.h` anzufassen, z. B.:

```ini
build_flags = ${env.build_flags} -D LCD_MODE=LCD_MODE_PARALLEL -D LCD_COLS=20 -D LCD_ROWS=4
```

## Build

Voraussetzung: [PlatformIO](https://platformio.org/) (VS-Code-Erweiterung oder `pip install platformio`).

Als Plattform wird **pioarduino** verwendet (Arduino-ESP32-Core 3.x). Die offizielle `espressif32`-Plattform unterstützt Arduino auf dem C6 nur eingeschränkt. Die Plattform-URL steht schon in `platformio.ini`.

```bash
# ESP32-C6 (Standard)
pio run
pio run -t upload
pio device monitor

# falls es doch ein ESP32-C3 ist
pio run -e esp32c3 -t upload
```

Board-Variante prüfen, wenn unklar: Beim Upload meldet esptool den erkannten Chip. Passt das Environment nicht, bricht es mit einer Meldung wie „This chip is ESP32-C3, not ESP32-C6“ ab. Dann einfach das andere Environment (`-e esp32c3`) verwenden.

### USB-Ports beim nanoESP32-C6

Das Board hat zwei USB-C-Buchsen: eine am USB-UART-Wandler (CH343) und eine am nativen USB des C6. In `platformio.ini` ist **USB-CDC on boot** aktiviert (`ARDUINO_USB_CDC_ON_BOOT=1`), deshalb erscheint `Serial` am **nativen USB-Port**. Flashen geht über beide Ports. Soll die serielle Ausgabe über den UART-Port laufen, `ARDUINO_USB_CDC_ON_BOOT=0` setzen.

Klappt der Upload nicht: BOOT-Taste halten, RESET kurz drücken, BOOT loslassen, dann erneut hochladen.

### Bibliotheken

PlatformIO lädt sie automatisch:

- `adafruit/DHT sensor library`
- `adafruit/Adafruit Unified Sensor`
- `marcoschwartz/LiquidCrystal_I2C` (I²C-Backpack)
- `arduino-libraries/LiquidCrystal` (parallel)

## Projektstruktur

```
include/config.h   Pins und Parameter
src/main.cpp       setup/loop, Anzeige-Logik, serielle Ausgabe
src/Pump.*         PWM, Softstart, Intervall-Zustandsmaschine
src/Climate.*      DHT22 lesen, letzten gültigen Wert halten, Fehler zählen
src/Display.*      LCD-Abstraktion (I²C oder parallel), flackerfreies Zeilen-Update
```

## Inbetriebnahme

1. Ohne Pumpe flashen und den seriellen Monitor öffnen: Kommen DHT22-Werte?
2. LCD prüfen. Bei I²C ohne Anzeige erst den Kontrast-Trimmer drehen, dann die Adresse `0x27`/`0x3F` testen.
3. Pumpe anschließen und den Strom bei 100 % und beim eingestellten Duty messen. Bei > 400 mA den Basiswiderstand auf 150 Ω ändern.
4. Läuft die Pumpe bei niedrigem Duty nach dem Softstart nicht weiter, `PUMP_DUTY_PERCENT` erhöhen oder `PUMP_KICK_MS` verlängern.
5. Resettet der ESP32 beim Pumpenstart, den Elko vergrößern bzw. näher an die Pumpe setzen.

## Offene Punkte

- [ ] Board-Variante endgültig klären (C3 oder C6)
- [ ] Display-Typ und Anschluss bestätigen (I²C oder parallel, 16×2 oder 20×4)
- [ ] Pumpenstrom an 5 V messen, ggf. Basiswiderstand anpassen
- [ ] Luftfilter am Pumpeneingang (Watte oder Spritzenfilter) gegen Fremdkeime und Essigfliegen
