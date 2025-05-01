/*
 * Autore: Alessandro Valenti
 * Data: 01 Maggio 2025
 *
 * Versione 0.1
 *
 * Questo codice è rilasciato sotto licenza open source.
 * Può essere utilizzato, modificato e distribuito liberamente
 * secondo i termini delle licenze libere, GPL.
 */


/*
 *  libreria per la gestione dei font U8g2
 *  in modo leggibile utilizzo 
 *  setFont(fontname, size)
 *  args: fontname -> Arial, Courier, Times
          size -> 8, 10, 12, 14, 18, 24
*/

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <U8g2lib.h>
#include <Arduino.h>

// Mappa font leggibili per l'utente

struct FontMapping {
  const char* nome;
  int size;
  const uint8_t* font;
};

// Lista statica dei font disponibili
static const FontMapping fontMappings[] = {
  {"Arial", 8, u8g2_font_helvR08_tf},
  {"Arial", 10, u8g2_font_helvR10_tf},
  {"Arial", 12, u8g2_font_helvR12_tf},
  {"Arial", 14, u8g2_font_helvR14_tf},
  {"Arial", 18, u8g2_font_helvR18_tf},
  {"Arial", 24, u8g2_font_helvR24_tf},
  {"Courier", 8, u8g2_font_courR08_tf},
  {"Courier", 10, u8g2_font_courR10_tf},
  {"Courier", 12, u8g2_font_courR12_tf},
  {"Courier", 14, u8g2_font_courR14_tf},
  {"Courier", 18, u8g2_font_courR18_tf},
  {"Courier", 24, u8g2_font_courR24_tf},
  {"Times", 8, u8g2_font_ncenR08_tf},
  {"Times", 10, u8g2_font_ncenR10_tf},
  {"Times", 12, u8g2_font_ncenR12_tf},
  {"Times", 14, u8g2_font_ncenR14_tf},
  {"Times", 18, u8g2_font_ncenR18_tf},
  {"Times", 24, u8g2_font_ncenR24_tf},
  // Aggiungi altre mappature se necessario
};

class DisplayManager {

private:
  U8G2* u8g2;
  uint8_t cursorX = 0;
  uint8_t cursorY = 10;
  uint8_t lineHeight = 12;

public:
  DisplayManager(U8G2 &display) {
    u8g2 = &display;
  }

  void begin() {
    u8g2->begin();
    clearScreen();
    setFont("Arial", 10);
  }

void setFont(const String& nome, int size) {
  for (const auto& mapping : fontMappings) {
    if (nome.equalsIgnoreCase(mapping.nome) && size == mapping.size) {
      u8g2->setFont(mapping.font);
      return;
    }
  }
  // Font non trovato, impostare un font predefinito
  u8g2->setFont(u8g2_font_6x10_tf);
}

  void clearScreen() {
    u8g2->clearBuffer();
    cursorX = 0;
    cursorY = 10;
  }

  void print(const String &text) {
    u8g2->drawStr(cursorX, cursorY, text.c_str());
    cursorX += u8g2->getStrWidth(text.c_str());
        u8g2->sendBuffer();

  }

  void println(const String &text) {
    print(text);
    cursorX = 0;
    cursorY += lineHeight;
        u8g2->sendBuffer();

  }

};

#endif