#include <Wire.h>
#include <U8g2lib.h>

// SH1106, I2C, buffer pieno, senza pin reset
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(
  U8G2_R0,     // rotazione normale
  U8X8_PIN_NONE // nessun pin di reset
);

void setup() {
  u8g2.begin();                 // inizializza display
  u8g2.clearBuffer();           // cancella buffer interno
  u8g2.setFont(u8g2_font_ncenB08_tr); // scegli font
  u8g2.drawStr(0, 24, "SH1106 OLED OK!");
  u8g2.sendBuffer();            // disegna tutto
}

void loop() {
  // puoi aggiornare il display qui
}