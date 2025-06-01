/*
 * Adattatore font per U8g2 basato su FontRegistry generico
 * Autore: Alessandro Valenti
 * Data: 01 Maggio 2025
 */

#ifndef FONT_U8G2_ADAPTER_H
#define FONT_U8G2_ADAPTER_H

#include "font_library_core.h"
#include <U8g2lib.h>

class FontRegistryU8g2 : public FontRegistry {
public:
  FontRegistryU8g2() {
    // ===== Helvetica (helv) - Arial Style =====
    //add("Arial", 6, u8g2_font_helvR06_tf);
    add("Arial", 8, u8g2_font_helvR08_tf);
    add("Arial", 10, u8g2_font_helvR10_tf);
    add("Arial", 12, u8g2_font_helvR12_tf);
    add("Arial", 14, u8g2_font_helvR14_tf);
    add("Arial", 18, u8g2_font_helvR18_tf);
    add("Arial", 24, u8g2_font_helvR24_tf);

    /* ===== Times (ncen) - Times New Roman Style =====
    add("Times", 8, u8g2_font_ncenR08_tf);
    add("Times", 10, u8g2_font_ncenR10_tf);
    add("Times", 12, u8g2_font_ncenR12_tf);
    add("Times", 14, u8g2_font_ncenR14_tf);
    add("Times", 18, u8g2_font_ncenR18_tf);
    add("Times", 24, u8g2_font_ncenR24_tf);

    // ===== Courier (cour) =====
    add("Courier", 8, u8g2_font_courR08_tf);
    add("Courier", 10, u8g2_font_courR10_tf);
    add("Courier", 12, u8g2_font_courR12_tf);
    add("Courier", 14, u8g2_font_courR14_tf);
    add("Courier", 18, u8g2_font_courR18_tf);
    add("Courier", 24, u8g2_font_courR24_tf);

    // ===== Monospace =====
    add("Mono", 6, u8g2_font_5x7_tf);
    add("Mono", 8, u8g2_font_6x10_tf);
    add("Mono", 9, u8g2_font_7x13_tf);
    add("Mono", 10, u8g2_font_8x13_tf);
    add("Mono", 12, u8g2_font_9x15_tf);
    add("MonoBold", 10, u8g2_font_9x15B_tf);

    // ===== Terminal Style =====
    add("Terminal", 10, u8g2_font_8x13B_tf);
    add("Terminal", 12, u8g2_font_9x18_tf);

    // ===== Symbols =====
    add("Symbols", 10, u8g2_font_open_iconic_all_1x_t);
    add("SymbolsSolid", 16, u8g2_font_open_iconic_all_2x_t);
    add("Weather", 16, u8g2_font_open_iconic_weather_2x_t);
    add("Arrows", 16, u8g2_font_open_iconic_arrow_2x_t);
    add("App", 16, u8g2_font_open_iconic_app_2x_t);

    // ===== LCD/Segment Display Style =====
    add("7Segment", 24, u8g2_font_7x14B_tf);
    add("LCD", 24, u8g2_font_logisoso24_tf);

    // ===== Pixel/Minimal =====
    add("Pixel", 5, u8g2_font_4x6_tf);
    add("Pixel", 6, u8g2_font_5x7_tf);
    add("Pixel", 8, u8g2_font_6x12_tf);

    // ===== Decorative / Script =====
    add("Script", 12, u8g2_font_cu12_tr);
    add("Script", 14, u8g2_font_cu12_tn);
    */
  }

  const uint8_t* getFont(const char* name, int size) {
    const FontMeta* f = find(name, size);
    return f ? static_cast<const uint8_t*>(f->fontPtr) : u8g2_font_6x10_tf;
  }
};

#endif
