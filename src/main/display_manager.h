/*
 * Autore: Alessandro Valenti
 * Data: 01 Maggio 2025
 *
 * Versione 0.2 (con FontAdapter)
 *
 * Questo codice è rilasciato sotto licenza open source.
 * Può essere utilizzato, modificato e distribuito liberamente
 * secondo i termini delle licenze libere, GPL.
 */

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <U8g2lib.h>
#include <Arduino.h>
#include <battery.h>
#include <WiFi.h>
#include "font_u8g2_adapter.h"

class DisplayManager {
private:
  U8G2* u8g2;
  Battery* batteria;
  FontRegistryU8g2 fontRegistry;
  uint8_t cursorX = 0;
  uint8_t cursorY = 20; // iniziare sotto la barra di stato
  uint8_t lineHeight = 12;

public:
  DisplayManager(U8G2 &display, Battery &batt)
    : u8g2(&display), batteria(&batt) {}

  void begin() {
    u8g2->begin();
    setFont("Arial", 10);
    aggiornaBarraStato();
  }

  void setFont(const String& nome, int size) {
    const uint8_t* f = fontRegistry.getFont(nome.c_str(), size);
    u8g2->setFont(f);
  }

  void aggiornaBarraStato(const String& wifi = "") {
    setFont("Arial", 8);
    u8g2->clearBuffer();
    cursorX = 0;
    cursorY = 10;
    int perc = batteria->percentualeBatteria();
    drawWifiBars(u8g2, 5, 10);
    drawBatteryBars(u8g2, 75, 0, perc);
    cursorY = 20; // riga successiva per contenuto utente
    u8g2->sendBuffer();
    setFont("Arial", 10);
  }

  void clearScreen() {
    aggiornaBarraStato();
    cursorX = 0;
    cursorY = 25;
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

  void drawWifiBars(U8G2* u8g2, int x, int y) {
    if (WiFi.status() == WL_CONNECTED) {
      u8g2->drawBox(x, y - 2, 2, 2);
      u8g2->drawBox(x + 4, y - 4, 2, 4);
      u8g2->drawBox(x + 8, y - 6, 2, 6);
      u8g2->drawBox(x + 12, y - 8, 2, 8);
    } else {
      u8g2->drawLine(x, y - 6, x + 10, y);
      u8g2->drawLine(x, y, x + 10, y - 6);
    }
  }

  void drawBatteryBars(U8G2* u8g2, int x, int y, float percent) {
    int w = 24, h = 10;
    int bars = round(percent / 20);
    u8g2->drawFrame(x, y, w, h);
    u8g2->drawBox(x + w, y + 3, 2, 4);
    for (int i = 0; i < bars; i++) {
      u8g2->drawBox(x + 2 + i * 4, y + 2, 3, h - 4);
    }
    u8g2->setFont(u8g2_font_6x10_tr);
    u8g2->setCursor(x + w + 5, y + h - 1);
    u8g2->print((int)percent);
    u8g2->print("%");
  }
};

#endif
