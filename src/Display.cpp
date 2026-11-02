#include "Display.h"

#if LCD_MODE == LCD_MODE_I2C
  #include <Wire.h>
  #include <LiquidCrystal_I2C.h>
  static LiquidCrystal_I2C lcd(LCD_I2C_ADDR, LCD_COLS, LCD_ROWS);
#elif LCD_MODE == LCD_MODE_PARALLEL
  #include <LiquidCrystal.h>
  // R/W liegt fest auf GND -> Konstruktor ohne RW-Pin
  static LiquidCrystal lcd(PIN_LCD_RS, PIN_LCD_EN,
                           PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7);
#else
  #error "LCD_MODE in config.h muss LCD_MODE_I2C oder LCD_MODE_PARALLEL sein."
#endif

void Display::begin() {
#if LCD_MODE == LCD_MODE_I2C
    // Eigene Pins setzen; das spätere Wire.begin() in lcd.init() erkennt den
    // bereits gestarteten Bus und lässt die Pins unverändert.
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    lcd.init();
    lcd.backlight();
#else
    lcd.begin(LCD_COLS, LCD_ROWS);
#endif
    clear();
}

void Display::clear() {
    lcd.clear();
    for (uint8_t r = 0; r < LCD_ROWS; r++) {
        memset(_shadow[r], ' ', LCD_COLS);
        _shadow[r][LCD_COLS] = '\0';
    }
}

void Display::printLine(uint8_t row, const char *text) {
    if (row >= LCD_ROWS) return;

    char buf[LCD_COLS + 1];
    size_t i = 0;
    for (; i < LCD_COLS && text[i] != '\0'; i++) buf[i] = text[i];
    for (; i < LCD_COLS; i++) buf[i] = ' ';
    buf[LCD_COLS] = '\0';

    if (memcmp(buf, _shadow[row], LCD_COLS) == 0) return;   // unverändert
    memcpy(_shadow[row], buf, LCD_COLS + 1);

    lcd.setCursor(0, row);
    lcd.print(buf);
}

void Display::printLR(uint8_t row, const char *left, const char *right) {
    char buf[LCD_COLS + 1];
    const size_t lenL = strlen(left);
    const size_t lenR = strlen(right);

    memset(buf, ' ', LCD_COLS);
    buf[LCD_COLS] = '\0';

    memcpy(buf, left, lenL < LCD_COLS ? lenL : LCD_COLS);
    if (lenR <= LCD_COLS) {
        // rechter Teil hat Vorrang, falls es zu eng wird
        memcpy(buf + LCD_COLS - lenR, right, lenR);
    }
    printLine(row, buf);
}
