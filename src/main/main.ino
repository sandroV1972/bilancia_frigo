/*
 * Autore: Alessandro Valenti
 * Data: 12 marzo 2025
 * Versione: 0.1
 *
 * Questo codice è rilasciato sotto licenza open source (GPL).
 * Può essere utilizzato, modificato e distribuito liberamente.
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <credentials.h>  // Credenziali WiFi
#include <Wire.h>
#include <U8g2lib.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <display_manager.h>
#include <Arduino.h>
#include <led_rgb_controller.h>
#include <battery.h>
#include "HX711.h"

// Inizializza la cella di carico (HX711)
HX711 loadcell;
const int LOADCELL_DOUT_PIN = 34;
const int LOADCELL_SCK_PIN = 35;
int ultimo_peso = 0;
const long LOADCELL_OFFSET = 50682624;
const long LOADCELL_DIVIDER = 5895655;

// Inizializza la lettura batteria sul pin 32
Battery batteria(32);

// Display OLED SH1106 128X64 Pixels monocromatico con protocollo comunicazione I2C
U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);
// Oggetto DisplayManager 
DisplayManager screen(oled, batteria);

#define SCAN_PIN 4   // Pulsante scansione QR
#define RXD2 16      // Serial2 RX from QR scanner
#define TXD2 17      // Serial2 TX from QR scanner

WiFiClientSecure client;
LedRGB led(13, 12, 14);  // LED RGB: R=13, G=12, B=14
String apiUrl = "https://world.openfoodfacts.org/api/v0/product/";

/**
 * @brief Inizializza periferiche (WiFi, display, HX711, QR scanner, LED RGB).
 * - Entra in deep sleep se batteria < 5%
 * - Configura QR scanner in modalità comando
 * - Mostra stato su display
 * - Inizializza HX711
 */
void setup() {
  Serial.begin(115200);

  if (batteria.percentualeBatteria() <= 5) {
    esp_sleep_enable_timer_wakeup(60 * 1000000);
    esp_deep_sleep_start();
  }

  pinMode(SCAN_PIN, INPUT_PULLUP);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  delay(200);

  String mode = getWorkingMode();
  if (mode.indexOf("00210003") == -1) {
    Serial.println("Imposto modalità comando...");
    Serial2.print("~M00210003.\r\n");
    delay(200);
    Serial2.print("~MA5F0506A.\r\n");
    if (Serial2.available()) {
      Serial.println(Serial2.readStringUntil('\n'));
    }
  }

  screen.begin();
  screen.clearScreen();
  screen.setFont("Times", 10);
  screen.println("Init...");
  screen.println("Connessione WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
  }
  led.blue();
  screen.clearScreen();
  screen.println("Connesso: ");
  screen.println(WiFi.SSID());

  Serial2.print("~T.\r\n");
  delay(500);
  if (Serial2.available()) {
    screen.clearScreen();
    screen.println("QR OK!");
    delay(500);
    screen.clearScreen();
  } else {
    screen.println("Errore QR!");
  }

  loadcell.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  loadcell.set_scale(LOADCELL_DIVIDER);
  loadcell.set_offset(LOADCELL_OFFSET);
}

/**
 * @brief Loop principale: gestisce scansione QR e lettura peso.
 */
void loop() {
  if (digitalRead(SCAN_PIN) == HIGH) {
    screen.clearScreen();
    screen.println("Scan...");
    String code = scanQRCode();
    screen.println(code);
    if (code != "") {
      Serial.println("Codice letto: " + code);
      fetchProductData(code);
    } else {
      screen.println("NO CODE");
    }
  }

  int peso = loadcell.get_units(10);
  if (peso != 0 && peso != ultimo_peso) {
    Serial.print("Peso: ");
    Serial.println(peso + " - " + ultimo_peso);
    ultimo_peso = peso;
  }
  delay(500);
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

/**
 * @brief Avvia la scansione QR e restituisce il codice letto.
 * @param timeout Tempo massimo di attesa in ms (default: 3000).
 * @return String contenente il codice numerico, oppure vuota.
 */
String scanQRCode(unsigned long timeout = 3000) {
  led.yellow();
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
 * @brief Recupera i dati prodotto da OpenFoodFacts tramite codice a barre.
 * @param code Codice numerico del prodotto.
 */
void fetchProductData(String code) {
  if (WiFi.status() == WL_CONNECTED) {
    String url = apiUrl + code + ".json";
    Serial.println("➡️ URL: " + url);

    client.setInsecure();
    HTTPClient http;
    http.begin(client, url);
    int httpCode = http.GET();

    if (httpCode == 200) {
      Serial.println("HTTP OK → provo parsing JSON");
      String response = http.getString();
      Serial.println("JSON ricevuto:");
      Serial.println(response);
      parseJSON(response);
    } else {
      Serial.print("HTTP Code: ");
      Serial.println(httpCode);
      screen.clearScreen();
      screen.println("Errore HTTP");
    }
    http.end();
  } else {
    Serial.println("WiFi Disconnected!");
    screen.clearScreen();
    screen.println("No WiFi");
  }
}

/**
 * @brief Parsing del JSON da OpenFoodFacts per estrarre nome, marca, peso.
 * @param jsonResponse Risposta JSON completa.
 */
void parseJSON(const String& jsonResponse) {
  DynamicJsonDocument doc(24576);
  DeserializationError error = deserializeJson(doc, jsonResponse);

  if (error) {
    Serial.print("⚠️ JSON Parsing failed: ");
    Serial.println(error.c_str());
    screen.clearScreen();
    screen.println("JSON error");
    return;
  }

  int status = doc["status"] | 0;
  if (status == 0) {
    Serial.println("Prodotto non trovato");
    screen.clearScreen();
    screen.println("NOT FOUND");
    return;
  }

  led.green();
  JsonObject product = doc["product"];
  String name = product["product_name"] | "Sconosciuto";
  String brand = product["brands"] | "Sconosciuto";
  String quantity = product["product_quantity"] | "N/D";

  screen.clearScreen();
  screen.println(name);
  screen.println(brand);
  screen.println("Peso: " + quantity + "g");
}
