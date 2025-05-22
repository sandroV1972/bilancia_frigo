#ifndef QR_FUNC_H
#define QR_FUNC_H


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

#endif