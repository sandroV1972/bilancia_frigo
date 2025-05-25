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
#include <HTTPClient.h>
#include "HX711.h"
#include "id_billancia.h"
#include "led_rgb_controller.h"
#include <Preferences.h>
#include <PubSubClient.h>
#include "qr_func.h"
#include "QRCodeGenerator.h"
#include <U8g2lib.h>
#include <UniversalTelegramBot.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include <Wire.h>
#define SCAN_PIN 5   // Pulsante scansione QR
#define RXD2 16      // Serial2 RX from QR scanner
#define TXD2 17      // Serial2 TX from QR scanner
#define RESET_PIN 4
#define RESET_HOLD_TIME 5000 // ms (5 secondi)

Preferences prefs;
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
const int LOADCELL_DOUT_PIN = 26;
const int LOADCELL_SCK_PIN = 25;
float lastPeso = 0.00;
unsigned long lastTime = 0;
int lettureUguali = 0;
const unsigned long interval = 500;  // 0.5 secondi = 500 ms
// Inizializza la lettura batteria sul pin 32
Battery batteria(32);
// Display OLED SH1106 128X64 Pixels monocromatico con protocollo comunicazione I2C
U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);
// Oggetto DisplayManager 
DisplayManager screen(oled, batteria);
LedRGB led(13, 12, 14);  // LED RGB: R=13, G=12, B=14
// URL used to rtreive JSON of scanned products
String apiUrl = "https://world.openfoodfacts.org/api/v0/product/";
const String topic = "bilancia/" + String(device_id);
String prodName = "";
float original_weight = 0;
float peso = 0;
TaskHandle_t taskBilancia;
float ultimo_peso = 0;
int stazionario_counter = 0;

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

  pinMode(RESET_PIN, INPUT_PULLUP);
  
  prefs.begin("bilancia", false);  // apre namespace "bilancia" in R/W
  prefs.begin("wifi", false); // apre namespace "wifi" in R/W

  String ssid_salvato = prefs.getString("ssid", "");
  String password_salvato = prefs.getString("password", "");
  String ssid_name = "BILANCIA" + String(device_id);

  Serial.println(ssid_salvato);
    // Parametri namespace BILANCIA in memoria
  // nome       | type (default)  | descrizione
  // _________________________________________________________________________
  // prodotto.  - String (N\A).   - nome del prodotto
  // marca.     - String (N\A).   - marca del prodotto
  // peso.      - float (0.0).    - peso del prodotto pieno
  //
  prodName = prefs.getString("nome", "N/A");
  String prodBrand = prefs.getString("marca", "N/A");
  float prodWeight = prefs.getFloat("peso", 0.0);

  Serial.println(prodName + prodBrand + prodWeight);
  // OLED Monitor setup
  screen.begin();
  screen.clearScreen();

  if (digitalRead(RESET_PIN) == HIGH) {
    Serial.println("RESET");
    prefs.clear();
    prefs.end();
    delay(500);
    // Disconnetti da qualsiasi rete Wi-Fi attiva
     WiFi.disconnect(true); // 'true' cancella le credenziali Wi-Fi salvate in flash (NVS) 
    // Metti il modulo Wi-Fi in modalità spenta
    WiFi.mode(WIFI_OFF);
    wifimanager.resetSettings();
    Serial.println("🔁 Reboot...");
    delay(1000);
    ESP.restart();

  }
  // se memorizzato una SSID prova a collegarsi per 10 secondi altrimenti mostra comunque il QR del portale
  if (ssid_salvato != "") {
    screen.println("Connetto a " + ssid_salvato);
    WiFi.begin(ssid_salvato.c_str(),password_salvato.c_str());
    unsigned long startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) {
      delay(500);
    }
  }

  if (WiFi.status() != WL_CONNECTED) {
    screen.clearScreen();
    screen.print("SSID:" +ssid_name);
    if(!wifimanager.autoConnect(ssid_name.c_str())) {
      screen.clearScreen();
      screen.print("No Connection");   
    } else {
        // Salva le credenziali correnti (SSID e Password) nella NVS
        // WiFiManager di per sé salva già le credenziali, ma questo dimostra come potresti gestirle
        // per altri scopi o per assicurarti che siano persistenti indipendentemente
        // dalle future inizializzazioni di WiFiManager.
        prefs.putString("ssid", WiFi.SSID());
        prefs.putString("password", WiFi.psk());
        prefs.end();
    }
  }

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
  Serial.println(mode);
  if (mode.indexOf("00210003") == -1) {
    Serial2.print("~M00210003.\r\n");
    delay(200);
    Serial2.print("~MA5F0506A.\r\n");
    if (Serial2.available()) {
      Serial.println("Serial2 Available");
    } else {
      Serial.println("Serial2 NOT Available");
    }
  }
  Serial.println("MQTT...");
  client.setServer(mqtt_server, 1883);
  //client.setCallback(callback);
  
  Serial.println("LED");
  // RGB Led setup
  led.blue();
  
  Serial.println("Screen");

  screen.clearScreen();
  screen.println("Connesso: ");
  screen.println(WiFi.SSID());

  Serial2.print("~T.\r\n");
  delay(500);
  if (Serial2.available()) {
    screen.println("QR OK!");
    delay(500);
  } else {
    screen.println("Errore QR!");
  }

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
    // Pulsante premuto
  if (digitalRead(RESET_PIN) == HIGH) {
    Serial.println("RESET");
    if (!resetInProgress) {
      resetPressStart = millis();
      resetInProgress = true;
    } else if (millis() - resetPressStart >= RESET_HOLD_TIME) {
      Serial.println("🧨 Reset a lungo - Cancello le preferenze...");
      Preferences prefs;
      prefs.begin("bilancia", false);
      prefs.clear();
      prefs.end();

      delay(500);
      Serial.println("🔁 Reboot...");
      ESP.restart();
    }
  } else {
    resetInProgress = false;  // Se rilasciato prima dei 5s
  }

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
      fetchProductData(code);
    } else {
      screen.println("NO CODE");
    }
  }
  delay(1000);
}

// === Task da eseguire su Core 0 ===
void taskPeso(void* parameter) {
  while (true) {
    float peso = loadcell.get_units();
    peso = peso - 183.65;
    Serial.print("Peso letto: ");
    Serial.println(peso);

    // Controllo cambiamento peso
    if (abs(peso - lastPeso) < 15.0) {
      lettureUguali++;
      if (lettureUguali >= 2) {
        Serial.println("Peso stabile, vado in sleep per 30s...");
        screen.sleep();
        vTaskDelay(pdMS_TO_TICKS(100));  // breve attesa prima dello sleep
        esp_sleep_enable_timer_wakeup(30 * 1000000ULL); // 30 sec
        esp_deep_sleep_start();
      }
    } else {
      lettureUguali = 0;
    }

    lastPeso = peso;
    vTaskDelay(pdMS_TO_TICKS(10000)); // Aspetta 10s
  }
}


/**
 * @brief Recupera i dati prodotto da OpenFoodFacts tramite codice a barre.
 * @param code Codice numerico del prodotto.
 */
void fetchProductData(String code) {
  if (WiFi.status() == WL_CONNECTED) {
    String url = apiUrl + code + ".json";
    Serial.println(url);
    wificlient.setInsecure();
    HTTPClient http;
    http.begin(wificlient, url);
    int httpCode = http.GET();

    if (httpCode == 200) {
      String response = http.getString();
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
}

/**
 * @brief Parsing del JSON da OpenFoodFacts per estrarre nome, marca, peso.
 * @param jsonResponse Risposta JSON completa.
 */
void parseJSON(const String& jsonResponse) {
  DynamicJsonDocument doc(24576);
  DeserializationError error = deserializeJson(doc, jsonResponse);

  if (error) {
    screen.clearScreen();
    screen.println("JSON error");
    return;
  }

  int status = doc["status"] | 0;
  if (status == 0) {
    screen.clearScreen();
    screen.println("NOT FOUND");
    return;
  }

  led.green();
  JsonObject product = doc["product"];
  String name = product["product_name"] | "Sconosciuto";
  prefs.putString("nome", name);
  String brand = product["brands"] | "Sconosciuto";
  prefs.putString("marca", brand);
  String quantity = product["product_quantity"] | "N/D";
  Serial.println(quantity);
  original_weight = quantity.toFloat();
  prefs.putFloat("peso", original_weight);

  if (quantity.length() > 0 && peso > 0.0) {
    prefs.putFloat("peso", peso);
  } else {
    prefs.putFloat("peso", 0.0);
  }

  screen.clearScreen();
  screen.println(name);
  screen.println(brand);
  screen.println("Peso: " + quantity + "g");

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    // Attempt to connect
      client.connect("ESP32scale");
  }
}
