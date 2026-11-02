# Essig-Belüftung

Kleine Steuerung für die Essigherstellung: Eine Mini-Luftpumpe belüftet ein Glas mit Essigansatz, ein AM2302 misst Temperatur und Luftfeuchte, die Werte kommen über die serielle Schnittstelle. Ein Display ist im ersten Entwurf nicht vorgesehen. Ein ESP32-C3 SuperMini schaltet die Pumpe über einen NPN-Transistor (kein Relais), mit PWM-Drosselung, Softstart und Intervallbetrieb.

## Funktionen

- **Temperatur/Luftfeuchte**: AM2302 wird alle 2 s gelesen. Bei Lesefehlern bleibt der letzte gültige Wert stehen; nach 3 Fehlversuchen in Folge meldet die serielle Ausgabe „Sensorfehler“.
- **PWM-Pumpe**: 1 kHz, 8 Bit, Arduino-ESP32-Core-3.x-API (`ledcAttach` / `ledcWrite`).
- **Softstart**: Beim Einschalten 500 ms lang 100 % Duty, danach der eingestellte Wert, damit der Motor auch bei niedrigem Duty anläuft.
- **Intervallbetrieb**: standardmäßig 1 min an und 5 min aus bei 60 % Duty. Läuft nicht-blockierend mit `millis()`, ohne `delay()` in der Schleife.
- **Temperaturwarnung**: Liegt die Temperatur außerhalb von 24–30 °C, hängt die serielle Ausgabe eine Warnung an.
- **Serielle Ausgabe**: alle 2 s Messwerte und Pumpenstatus (AN/AUS, Duty, Restzeit der Phase) mit 115200 Baud.

Serielle Ausgabe (Beispiel):

```
[   42000] T=26.4 C  RH=71.2 %  | Pumpe AN 60%, Rest 18 s
```

## Hardware / Stückliste

| Anz. | Bauteil | Bemerkung |
|---|---|---|
| 1 | ESP32-C3 SuperMini (USB-C) | wird über den 5V-Pin versorgt |
| 1 | Netzteil 230 V AC → 5 V DC, 3 A | versorgt ESP32 (5V-Pin) und Pumpe |
| 1 | 370er Mini-Luftpumpe, 3–9 V DC | an 5 V betrieben; ca. 200–400 mA, Anlauf kurz bis ~1 A (**nachmessen**) |
| 1 | NPN-Transistor BC337 (800 mA, 1 A Spitze) | Alternativen: 2N2222, S8050 (anderes Pinout!) |
| 1 | Widerstand 220 Ω | Basiswiderstand; bei Pumpenstrom > 400 mA: 150 Ω |
| 1 | Widerstand 10 kΩ | Pull-down Basis → GND (empfohlen, hält die Pumpe beim Booten aus) |
| 1 | Diode 1N4007 oder 1N5819 | Freilaufdiode, **Pflicht** |
| 1 | Elko 470–1000 µF, ≥ 10 V | Puffer an 5 V nahe der Pumpe (gegen Brown-out-Resets) |
| 1 | AM2302 (= DHT22) | an 3,3 V |
| 1 | Widerstand 10 kΩ | Pull-up an der AM2302-Datenleitung (bei Modulen oft schon drauf) |
| – | Schlauch, Ausströmerstein, Luftfilter | Watte oder Spritzenfilter am Pumpeneingang gegen Fremdkeime und Essigfliegen |

## Schaltung

### Stromversorgung

```
Netzteil +5V ──┬── ESP32 5V-Pin
               ├── Pumpe (+)
               └── Elko (+) 470–1000 µF ── GND

Netzteil GND ──── gemeinsame Masse: ESP32, Transistor-Emitter, AM2302, Elko (–)
```

**Alle GNDs gemeinsam verbinden.** Die Pumpe nicht aus dem 3,3-V-Regler des Boards versorgen, der 3,3-V-Pin des SuperMini reicht nur für den AM2302.

> **USB und Netzteil gleichzeitig:** Viele SuperMini-Boards haben keine Diode zwischen USB-5V und dem 5V-Pin. Ist beides angeschlossen, speisen sich Netzteil und PC gegenseitig. Zum Flashen und Debuggen deshalb das Netzteil abziehen (Pumpe läuft dann nicht) oder am eigenen Board nachsehen, ob eine Diode bestückt ist.

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

### AM2302

```
AM2302 Pin 1 (VCC)  ── 3,3 V
AM2302 Pin 2 (Data) ── GPIO (DHT) ── 10 kΩ ── 3,3 V
AM2302 Pin 3        ── nicht belegt
AM2302 Pin 4 (GND)  ── GND
```

Beim AM2302 mit Kabel (drei Litzen) gilt meist: rot = VCC, gelb = Data, schwarz = GND. Diese Ausführung hat den Pull-up oft schon eingebaut, dann entfällt der 10-kΩ-Widerstand. Im Zweifel mit dem Datenblatt bzw. der Beschriftung abgleichen.

## Pinbelegung

Alle Pins stehen in [`include/config.h`](include/config.h).

| Funktion | GPIO |
|---|---|
| Pumpe (PWM) | 4 |
| AM2302 Data | 5 |

Der SuperMini führt GPIO 0–10, 20 und 21 heraus. Frei bleiben sollten:

- **GPIO 2, 8, 9**: Strapping-Pins (8 hängt zusätzlich an der Onboard-LED, 9 an der BOOT-Taste)
- **GPIO 20/21**: UART0 (RX/TX)

## Konfiguration

Alles steht in `include/config.h`:

| Parameter | Default | Bedeutung |
|---|---|---|
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

## Build

Voraussetzung: [PlatformIO](https://platformio.org/) (VS-Code-Erweiterung oder `pip install platformio`).

Als Plattform wird **pioarduino** verwendet (Arduino-ESP32-Core 3.x). Die Pumpensteuerung nutzt die LEDC-API von Core 3.x (`ledcAttach`), die offizielle `espressif32`-Plattform bringt noch Core 2.x mit. Die Plattform-URL steht schon in `platformio.ini`. Als Board-Profil dient `esp32-c3-devkitm-1`, für den SuperMini gibt es kein eigenes.

```bash
pio run
pio run -t upload
pio device monitor
```

### USB beim SuperMini

Der SuperMini hat keinen USB-UART-Wandler, die USB-C-Buchse hängt direkt am nativen USB des C3. Deshalb ist in `platformio.ini` **USB-CDC on boot** aktiviert (`ARDUINO_USB_CDC_ON_BOOT=1`), sonst käme keine serielle Ausgabe am PC an. Nach einem Reset meldet sich der Port neu an, der Monitor verbindet sich dann kurz darauf wieder.

Klappt der Upload nicht: BOOT-Taste halten, RESET kurz drücken, BOOT loslassen, dann erneut hochladen.

### Bibliotheken

PlatformIO lädt sie automatisch:

- `adafruit/DHT sensor library`
- `adafruit/Adafruit Unified Sensor`

## Projektstruktur

```
include/config.h   Pins und Parameter
src/main.cpp       setup/loop, serielle Ausgabe
src/Pump.*         PWM, Softstart, Intervall-Zustandsmaschine
src/Climate.*      AM2302 lesen, letzten gültigen Wert halten, Fehler zählen
```

## Inbetriebnahme

1. Ohne Pumpe flashen und den seriellen Monitor öffnen: Kommen AM2302-Werte?
2. Pumpe anschließen und den Strom bei 100 % und beim eingestellten Duty messen. Bei > 400 mA den Basiswiderstand auf 150 Ω ändern.
3. Läuft die Pumpe bei niedrigem Duty nach dem Softstart nicht weiter, `PUMP_DUTY_PERCENT` erhöhen oder `PUMP_KICK_MS` verlängern.
4. Resettet der ESP32 beim Pumpenstart, den Elko vergrößern bzw. näher an die Pumpe setzen.

## Offene Punkte

- [ ] Display nachrüsten (später; Code dafür ist in der Git-History, Commit 33ab8b1)
- [ ] Pumpenstrom an 5 V messen, ggf. Basiswiderstand anpassen
- [ ] Luftfilter am Pumpeneingang (Watte oder Spritzenfilter) gegen Fremdkeime und Essigfliegen
