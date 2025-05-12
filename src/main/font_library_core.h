/*
 * Libreria Font Core per uso generico
 * Contiene mappatura font generica indipendente dalla libreria grafica
 * Autore: Alessandro Valenti
 * Data: 01 Maggio 2025
 */

#ifndef FONT_LIBRARY_CORE_H
#define FONT_LIBRARY_CORE_H

#include <Arduino.h>

struct FontMeta {
  const char* name;
  int size;
  const void* fontPtr;  // Puntatore generico al font (può essere U8g2, Adafruit GFX, etc.)
};

class FontRegistry {
public:
  FontRegistry() : count(0) {}

  // Aggiunge una nuova mappatura font
  void add(const char* name, int size, const void* ptr) {
    if (count < MAX_FONTS) {
      fonts[count++] = { name, size, ptr };
    }
  }

  // Trova un font dato nome e dimensione
  const FontMeta* find(const char* name, int size) const {
    for (int i = 0; i < count; ++i) {
      if (strcmp(name, fonts[i].name) == 0 && fonts[i].size == size) {
        return &fonts[i];
      }
    }
    return nullptr;
  }

  // Stampa tutti i font registrati
  void listAll() const {
    Serial.println("\n📋 Font registrati:");
    for (int i = 0; i < count; ++i) {
      Serial.print(" - ");
      Serial.print(fonts[i].name);
      Serial.print("  size: ");
      Serial.println(fonts[i].size);
    }
    Serial.println();
  }

private:
  static const int MAX_FONTS = 32;
  FontMeta fonts[MAX_FONTS];
  int count;
};

#endif
