// Dünne Hülle um das HD44780-LCD – wahlweise I²C-Backpack (PCF8574) oder
// parallel im 4-Bit-Modus (Auswahl über LCD_MODE in config.h).
//
// Zeilen werden zwischengespeichert und nur bei Änderung neu geschrieben,
// dadurch kein Flackern und kaum Buslast.
#pragma once

#include <Arduino.h>
#include "config.h"

// HD44780-Zeichen-ROM A00: 0xDF ist das Gradzeichen
constexpr char LCD_DEGREE = (char)0xDF;

class Display {
public:
    void begin();
    void clear();

    // Zeile 'row' mit 'text' beschreiben, mit Leerzeichen auf LCD_COLS auffüllen.
    void printLine(uint8_t row, const char *text);

    // Text links, Text rechtsbündig – dazwischen Leerzeichen.
    void printLR(uint8_t row, const char *left, const char *right);

    uint8_t cols() const { return LCD_COLS; }
    uint8_t rows() const { return LCD_ROWS; }

private:
    char _shadow[LCD_ROWS][LCD_COLS + 1] = {};
};
