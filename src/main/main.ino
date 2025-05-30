/*
 * Autore: Alessandro Valenti
 * Data: 12 marzo 2025
 * Versione: 0.1
 *
 * Questo codice è rilasciato sotto licenza open source (GPL).
 * Può essere utilizzato, modificato e distribuito liberamente.
 */

#include <Arduino.h>
#include "ArduinoJson.h"
#include "battery.h"
#include "display_manager.h"
#include "globals.h"
#include <HTTPClient.h>
#include "HX711.h"
#include "id_bilancia.h"
#include "led_rgb_controller.h"
#include "nvs_flash.h"
#include <Preferences.h>
#include <PubSubClient.h>
#include "qr_func.h"
//#include "QRCodeGenerator.h"
#include <U8g2lib.h>
#include <UniversalTelegramBot.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include "wifi_manager.h"
#include <Wire.h>
#define BATTERY_PIN 32 // pin controllo livello batteria
#define SCAN_PIN 5   // Pulsante scansione QR
#define RXD2 16      // Serial2 RX from QR scanner
#define TXD2 17      // Serial2 TX from QR scanner
#define RESET_PIN 4
#define RESET_HOLD_TIME 5000 // ms (5 secondi)
#define LOADCELL_DOUT_PIN 26
#define LOADCELL_SCK_PIN 25

unsigned long resetPressStart = 0;
bool resetInProgress = false;
WebServer server;
WiFiManager wifimanager;
WiFiClientSecure wificlient; // Wifi Client
WiFiClient mqttclient;
// Add your MQTT Broker IP address, example:
const char* mqtt_server = "mqtt.atrent.it";
PubSubClient client(mqttclient);
HX711 loadcell; // Inizializza la cella di carico (HX711)

float lastPeso = 0.00;
unsigned long lastTime = 0;
int lettureUguali = 0;
const unsigned long interval = 500;  // 0.5 secondi = 500 ms
// Inizializza la lettura batteria sul pin 32
Battery batteria(BATTERY_PIN);
// Display OLED SH1106 128X64 Pixels monocromatico con protocollo comunicazione I2C
U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);
// Oggetto DisplayManager 
DisplayManager screen(oled, batteria);
LedRGB led(13, 12, 14);  // LED RGB: R=13, G=12, B=14
// URL used to rtreive JSON of scanned products
String apiUrl = "https://world.openfoodfacts.org/api/v0/product/";
const String topic = "bilancia/" + String(device_id);
const String ssid_captive = "BILANCIA-"+String(device_id);

float original_weight = 0;
float peso = 0;
TaskHandle_t taskBilancia;

Preferences prefs;
String prodName;
String prodBrand;
float prodWeight;
String ssid;
String password;
bool inviamqtt;

/**
 * @brief Inizializza periferiche (WiFi, display, HX711, QR scanner, LED RGB).
 * - Entra in deep sleep se batteria < 5%
 * - Configura QR scanner in modalità comando
 * - Mostra stato su display
 * - Inizializza HX711
 */
void setup() {

  Serial.begin(115200);
  delay(500);

  // Se al reboot il tasto reset ROSSO viene è attivato il modulo si resetta
  pinMode(RESET_PIN, INPUT_PULLUP);
  if (digitalRead(RESET_PIN) == HIGH) {
    Serial.println("RESET");
    resetNVS();
    WiFi.mode(WIFI_OFF);
    wifimanager.resetSettings();
    Serial.println("🔁 Reboot...");
    delay(1000);
    ESP.restart();
  }
  // OLED Monitor setup
  screen.begin();
  screen.clearScreen();
  
  if (!wifiReady()) {
    screen.println("Connect to: ");
    screen.println(ssid_captive);

    initWifi();
    WiFi.mode(WIFI_OFF);   // spegne fisicamente il modulo WiFi
  }

  // QR code scanner setup 
  pinMode(SCAN_PIN, INPUT_PULLUP);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  delay(200);

  if (prefProdottoIsEmpty()) {
    screen.clearScreen();
    screen.println("ATTIVA");
    screen.println("SCAN QR");
    delay(100);
    /*
    // Command continuous mode sleep setting
    //  Turn off sleep ~M00220000.
    //  Start sleep ~M00220001.
    */
    String mode = getWorkingMode();
    Serial.println(mode);
    prepareQR();  
    prodName = prefs.getString("nome", "");
    prodBrand = prefs.getString("marca", "");
    prodWeight = prefs.getFloat("peso", 0.0);
    while (prefProdottoIsEmpty()) {
      led.blue();
      if (digitalRead(SCAN_PIN) == HIGH) {
        led.yellow();
        String code = scanQRCode();
        Serial.println(code);
        if (code != "") {
          fetchProductData(code);
          prodName = prefs.getString("nome", "");
          prodBrand = prefs.getString("marca", "");
          prodWeight = prefs.getFloat("peso", 0.0);
        }
      }
    }
  }

  led.green();

  loadcell.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  loadcell.set_scale(-1950.3);

  xTaskCreatePinnedToCore(
    taskPeso,        // funzione da eseguire
    "TaskBilancia",  // nome
    4096,            // stack size
    NULL,            // parametri
    1,               // priorità
    &taskBilancia,   // handle
    0                // Core 0
  );
}

/**
 * @brief Loop principale: gestisce scansione QR e lettura peso.
 */
void loop() {
  ////////
  if (inviamqtt) {
    WiFi.begin(ssid, password);
    Serial.print("Connessione WiFi");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("✅ Connesso");

    client.setServer(mqtt_server, 1883);

    Serial.print("Connessione MQTT...");
    if (!client.connect("ESP32bilancia")) {
      Serial.print("❌ Errore: ");
      Serial.println(client.state());
    } else {
      Serial.println("✅ Connesso MQTT");
       String messaggio = prodName + " in esaurimento";
      bool sent = client.publish(topic.c_str(), messaggio.c_str());
      Serial.println(sent ? "✅ Messaggio inviato" : "❌ Invio fallito");
    }
    inviamqtt = false;
  }
  ///////



    // Pulsante premuto per 5 secondo si resetta il device
  if (digitalRead(RESET_PIN) == HIGH) {
    Serial.println("RESET");
    if (!resetInProgress) {
      resetPressStart = millis();
      resetInProgress = true;
    } else if (millis() - resetPressStart >= RESET_HOLD_TIME) {
      Serial.println("🧨 Reset a lungo - Cancello le preferenze...");
      resetNVS();
      delay(500);
      Serial.println("🔁 Reboot...");
      ESP.restart();
    }
  } else {
    resetInProgress = false;  // Se rilasciato prima dei 5s
  }

  if (digitalRead(SCAN_PIN) == HIGH) {
    screen.clearScreen();
    screen.println("Scan...");
    led.yellow();
    String code = scanQRCode();
    screen.println("mode: " + code);
    if (code != "") {
      fetchProductData(code);
    } else {
      screen.println("NO CODE");
    }
  }
  delay(1000);
}


// === Task da eseguire su Core 0 ===
void taskPeso(void* parameter) {
  //client.setServer(mqtt_server, 1883); // sempre una volta fuori dal loop

  while (true) {
    inviamqtt = false;
    float peso = loadcell.get_units() - 183.65;
    Serial.println("📦 Peso letto: " + String(peso, 2));

    if (peso > 10 && peso < (prodWeight * 0.2)) {
      inviamqtt = true;
    }

    // Output su display
    screen.clearScreen();
    if (peso < 10) {
      screen.println(prodName);
      screen.println(prodBrand);
      screen.println(String(prodWeight, 2));
      screen.println("VUOTA");
    } else {
      screen.println("W: " + String(peso, 2));
      led.green();
    }

    // sleep se peso stabile
    lettureUguali++;
    if (lettureUguali >= 2) {
      Serial.println("Peso stabile, sleep 30s...");
      screen.sleep();
      vTaskDelay(pdMS_TO_TICKS(100));
      esp_sleep_enable_timer_wakeup(30 * 1000000ULL);
      esp_deep_sleep_start();
    }

    vTaskDelay(pdMS_TO_TICKS(20000)); // ogni 20s
  }
}

/**
 * @brief Recupera i dati prodotto da OpenFoodFacts tramite codice a barre.
 * @param code Codice numerico del prodotto.
 */
void fetchProductData(String code) {
  if (!wifiReady()) {
    initWifi();
  }
  
  connectWiFi();

  if (WiFi.status() == WL_CONNECTED) {
    String url = apiUrl + code + ".json";
    Serial.println(url);
    wificlient.setInsecure();
    HTTPClient http;
    http.begin(wificlient, url);
    int httpCode = http.GET();

    if (httpCode == 200) {
      String response = http.getString();
      //Serial.println(response);
      parseJSON(response);
    } else {
      screen.clearScreen();
      screen.println("Errore HTTP");
    }
    http.end();
  } else {
    screen.clearScreen();
    screen.println("No WiFi");
  }

  disconnectWiFi();
}

/**
 * @brief Parsing del JSON da OpenFoodFacts per estrarre nome, marca, peso.
 * @param jsonResponse Risposta JSON completa.
 */
void parseJSON(const String& jsonResponse) {

    Serial.println("📦 JSON ricevuto:");
  Serial.println("📏 Dimensione: " + String(jsonResponse.length()));

  // Filtro per risparmiare memoria
  StaticJsonDocument<512> filter;
  filter["product"]["product_name"] = true;
  filter["product"]["brands"] = true;
  filter["product"]["product_quantity"] = true;

  DynamicJsonDocument doc(4096);  // adatto a risposta filtrata

  DeserializationError error = deserializeJson(doc, jsonResponse, DeserializationOption::Filter(filter));
  if (error) {
    Serial.println("❌ Errore nel parsing JSON:");
    Serial.println(error.c_str());
    return;
  }

  JsonObject product = doc["product"];
  String name = product["product_name"] | "Sconosciuto";
  String brand = product["brands"] | "Sconosciuto";
  float peso = product["product_quantity"].as<float>();

  Serial.println("✅ JSON OK:");
  Serial.println("Nome prodotto: " + name);
  Serial.println("Marca: " + brand);
  Serial.println("Peso: " + String(peso));

  prefs.begin("settings", false);
  prefs.putString("nome", name);
  prefs.putString("marca", brand);
  prefs.putFloat("peso", peso);
  prefs.end();

  screen.clearScreen();
  screen.println(name);
  screen.println(brand);
  screen.println(String(peso, 2));

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    // Attempt to connect
      client.connect("ESP32scale");
  }
}

void resetNVS() {
  esp_err_t err = nvs_flash_erase();
  if (err == ESP_OK) {
    Serial.println("🧹 NVS cancellato completamente!");
  } else {
    Serial.print("❌ Errore NVS erase: ");
    Serial.println(esp_err_to_name(err));
  }

  // Serve per riinizializzare NVS dopo l'erase
  nvs_flash_init();  
}
