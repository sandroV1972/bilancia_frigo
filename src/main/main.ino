/*
 * Autore: Alessandro Valenti
 * Data: 1 giugno 2025
 * Versione: 0.9
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
#include <U8g2lib.h>
#include <UniversalTelegramBot.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include "wifi_manager.h"
#include <Wire.h>

#define BATTERY_PIN 32 // pin controllo livello batteria
#define SCAN_PIN 5     // Pulsante scansione QR
#define RXD2 16        // Serial2 RX from QR scanner
#define TXD2 17        // Serial2 TX from QR scanner
#define RESET_PIN 4    // Pulsante reset
#define RESET_HOLD_TIME 5000 // ms (5 secondi di pressione pulsante reset per avviare il reset)
#define LOADCELL_DOUT_PIN 26 // data pin della cella di carico
#define LOADCELL_SCK_PIN 25  // clock pin della cella di carico

bool resetInProgress = false;
long resetPressStart = 0;
WebServer server;
WiFiManager wifimanager;
WiFiClientSecure wificlient; // Wifi Client
WiFiClient mqttclient;
const char* mqtt_server = "mqtt.atrent.it";
PubSubClient client(mqttclient);
HX711 loadcell; // Inizializza la cella di carico (HX711)

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
// URL per JSON prodotti
String apiUrl = "https://world.openfoodfacts.org/api/v0/product/";
const String topic = "bilancia/" + String(device_id);
const String ssid_captive = "BILANCIA-"+String(device_id);

float lastPeso = 0.01;
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
  // Imposta intensità globale (0.0 = spento, 1.0 = massimo)
  led.setIntensity(0.1);   // Solo il 10% della luminosità

  // Se al reboot il tasto reset ROSSO è attivato il modulo si resetta
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
  

  //
  // Se non sono state memorizzate SSID locale e credenziali
  // AVVIA CAPTIVE PORTAL per collegarsi a un nuova rete locale
  // 
  if (!wifiReady()) {
    screen.println("Connect to: ");
    screen.println(ssid_captive);
    // Inizializza il CAPTIVE PORTAL 
    initWifi();
    WiFi.mode(WIFI_OFF);   // spegne fisicamente il modulo WiFi
  }

  // QR code scanner setup 
  pinMode(SCAN_PIN, INPUT_PULLUP);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  delay(200);

  // Se non c'è un prodotto salvato nelle preferenze
  // si blocca fino a quando non viene scansionato un prodotto
  if (prefProdottoIsEmpty()) {
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


    //
    // Alll'accensione se il prodotto non è popolato nellle Preferences
    // la bilancia chiederà una scansione
    //
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

  // Led verde alla memorizzazione di un prodotto
  led.green();

  //
  // attiva la CELLA DI CARICO
  //
  loadcell.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  loadcell.set_scale(-1950.3);

  //
  // Inizializza il task parallelo per la cella di carcico e le pesate
  //
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
  //
  // controlla a ogni iterazion se deve inviare messaggio MQTT
  // la connesisone wifi e la connessione a MQTT server sono attive solo in caso si debba inviare messaggio 
  // al server
  //
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

    // Pulsante premuto per 5 secondo si resetta il device
  if (digitalRead(RESET_PIN) == HIGH) {
    Serial.println("RESET");
    // 
    // REGISTRO CHE IL PULSANTE È PREMUTO E IL MOMENTO DELLA PRESSIONE 
    //
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

  //
  // In caso di pressione del tasto per la scansione QR attiva la scansione 
  // e tenta di ricavare i dati via file JSON dal server staging openfoodfacts
  //

  if (digitalRead(SCAN_PIN) == HIGH) {
    vTaskSuspend(taskBilancia);
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
    lettureUguali = 0;
    vTaskSuspend(taskBilancia);

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
    float variazione = (fabs(peso) - fabs(lastPeso))/fabs(lastPeso);
    if (peso > 10 && peso < (prodWeight * 0.2) && (variazione > 0.10 && variazione < 1)) {
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
      screen.println(prodName);
      screen.println(prodBrand);
      screen.println("W: " + String(peso, 2));
      led.green();
    }

    // sleep se peso stabile
   
    Serial.println("variazione: " + String(variazione));
    if ((lastPeso != 0) && (variazione < 0.10)) {
      lettureUguali++;
    } else {
      lettureUguali = 0;
    }
    lastPeso = peso;
    if (lettureUguali >= 2) {
      lettureUguali = 0;
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
  WiFi.disconnect(true);
  connectWiFi();

  if (WiFi.status() == WL_CONNECTED) {
  String url = "https://world.openfoodfacts.net/api/v2/product/" + code + "json?fields=product_name,brands,product_quantity";
  Serial.println("URL: " + url);
IPAddress openfoodIP;
WiFi.hostByName("world.openfoodfacts.org", openfoodIP);
Serial.println(openfoodIP);
  wificlient.setInsecure(); // accetta qualunque certificato
  HTTPClient http;
  http.setTimeout(5000);  // timeout 5s
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  
  if (http.begin(wificlient, url)) {
    int httpCode = http.GET();
    Serial.printf("HTTP code: %d\n", httpCode);
    
    if (httpCode == 200) {
      String response = http.getString();
      parseJSON(response);
    } else {
      screen.clearScreen();
      screen.println("Errore HTTP: " + String(httpCode));
    }
    http.end();
  } else {
    Serial.println("Errore http.begin()");
  }
}else {
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
