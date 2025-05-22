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
#include <PubSubClient.h>
#include "HX711.h"
#include <qr_func.h>


HX711 loadcell; // Inizializza la cella di carico (HX711)
const int LOADCELL_DOUT_PIN = 26;
const int LOADCELL_SCK_PIN = 25;
float lastPeso = 0.00;
unsigned long lastTime = 0;
const unsigned long interval = 500;  // 0.5 secondi = 500 ms

// Inizializza la lettura batteria sul pin 32
Battery batteria(32);

// Display OLED SH1106 128X64 Pixels monocromatico con protocollo comunicazione I2C
U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);
// Oggetto DisplayManager 
DisplayManager screen(oled, batteria);

#define SCAN_PIN 5   // Pulsante scansione QR
#define RXD2 16      // Serial2 RX from QR scanner
#define TXD2 17      // Serial2 TX from QR scanner


WiFiClientSecure wificlient; // Wifi Client
WiFiClient mqttclient;
// Add your MQTT Broker IP address, example:
const char* mqtt_server = "mqtt.atrent.it";
PubSubClient client(mqttclient);

LedRGB led(13, 12, 14);  // LED RGB: R=13, G=12, B=14

// URL used to rtreive JSON of scanned products
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

  // Battery parameters
  if (batteria.percentualeBatteria() <= 5) {
    esp_sleep_enable_timer_wakeup(60 * 1000000); // Deep sleep for 60sec if battry very low
    esp_deep_sleep_start();
  }

  // QR code scanner setup 
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

  // OLED Monitor setup
  screen.begin();
  screen.clearScreen();
  screen.setFont("Times", 10); //set font
  screen.println("Init...");

  // WiFi connections
  screen.println("Connessione WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
  }

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // RGB Led setup
  led.blue();
  
  screen.clearScreen();
  screen.println("Connesso: ");
  screen.println(WiFi.SSID());

  loadcell.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  loadcell.set_scale(250.0f);
  loadcell.tare();

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
  loadcell.set_scale(250.0f);
  loadcell.tare();
}

/**
 * @brief Loop principale: gestisce scansione QR e lettura peso.
 */
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (digitalRead(SCAN_PIN) == HIGH) {
    screen.clearScreen();
    screen.println("Scan...");
    led.yellow();
    String code = scanQRCode();
    screen.println(code);
    if (code != "") {
      Serial.println("Codice letto: " + code);
      fetchProductData(code);
    } else {
      screen.println("NO CODE");
    }
  }
  int currentTime = millis();
  float peso = loadcell.get_units(1);
  if (peso > 10  && currentTime - lastTime > interval) {
    Serial.print("Peso: ");
    Serial.println(String(peso));
    lastTime = currentTime;
    if (peso > 2000) {
      Serial.println("Invio...");
      client.publish("test/messaggi", String(peso).c_str());
    } else if (lastPeso > 2000.00 && peso < 2000) {
      Serial.println("Invio...");
      client.publish("test/messaggi", "off");
    }
    lastPeso = peso;
  }
}


/**
 * @brief Recupera i dati prodotto da OpenFoodFacts tramite codice a barre.
 * @param code Codice numerico del prodotto.
 */
void fetchProductData(String code) {
  if (WiFi.status() == WL_CONNECTED) {
    String url = apiUrl + code + ".json";
    Serial.println("➡️ URL: " + url);

    wificlient.setInsecure();
    HTTPClient http;
    http.begin(wificlient, url);
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

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT

 
  if (String(topic) == "test/messaggi") {
    Serial.print("Changing output to ");
    if (isNumeric(messageTemp)){
      float num = messageTemp.toFloat();
      if(num > 500.00) {
         Serial.println("on");
        digitalWrite(LED_BUILTIN, HIGH);
      } 
    }
    if(messageTemp == "on"){
      Serial.println("on");
      digitalWrite(LED_BUILTIN, HIGH);
    }
    else if(messageTemp == "off"){
      Serial.println("off");
      digitalWrite(LED_BUILTIN, LOW);
    }
  }
}

bool isNumeric(const String& str) {
  if (str.length() == 0) return false;
  bool punto = false;
  for (unsigned int i = 0; i < str.length(); i++) {
    char c = str.charAt(i);
    if (i == 0 && (c == '-' || c == '+')) continue;
    if (c == '.') {
      if (punto) return false;
      punto = true;
      continue;
    }
    if (!isDigit(c)) return false;
  }
  return true;
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP32scale")) {
      Serial.println("Connesso a broker MQTT come ESP32Scale");
      client.subscribe("test/messaggi");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
