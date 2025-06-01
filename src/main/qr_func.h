#ifndef QR_FUNC_H
#define QR_FUNC_H

#include "globals.h"


/**
 * @brief Legge la modalità corrente del lettore QR scanner GM65.
 * @return String contenente il codice della modalità.
 */
String getWorkingMode() {
  Serial2.flush();
  Serial2.print("~Q0021.\r\n");
  delay(100);
  String response = "";
  if (Serial2.available()) {
    response = Serial2.readStringUntil('\n');
  }
  return response;
}
void prepareQR() {
  String mode = getWorkingMode();
  if (mode.indexOf("00210003") == -1) {
    Serial2.print("~M00210003.\r\n");
    delay(200);
    Serial2.print("~MA5F0506A.\r\n");
    //prova una lettura per due secondi
    Serial2.print("~M00B10020.\r\n");
    delay(200);
  }
}

/**
 * @brief Avvia la scansione QR e restituisce il codice letto.
 * @param timeout Tempo massimo di attesa in ms (default: 3000).
 * @return String contenente il codice numerico, oppure vuota.
 */
String scanQRCode(unsigned long timeout = 3000) {
  Serial2.flush();
  Serial2.print("~T.\r\n");

  unsigned long start = millis();
  String raw = "";

  while (millis() - start < timeout) {
    if (Serial2.available()) {
      raw = Serial2.readStringUntil('\n');
      raw.trim();
      int i = 0;
      while (i < raw.length() && !isDigit(raw[i])) i++;
      String codice = raw.substring(i);
      codice.trim();
      return codice;
    }
  }
  return "";
}

bool prefProdottoIsEmpty() {
  prefs.begin("settings", false);  // apre namespace "settings" in R/W
  prodName = prefs.getString("nome", "");
  prodBrand = prefs.getString("marca", "");
  prodWeight = prefs.getFloat("peso", 0.0);
  prefs.end();
  return (prodName.isEmpty());
}

#endif