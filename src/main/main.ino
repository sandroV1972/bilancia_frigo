/*
 * Autore: Alessandro Valenti
 * Data: 12 marzo 2025
 *
 * Versione 0.1
 *
 * Questo codice è rilasciato sotto licenza open source.
 * Può essere utilizzato, modificato e distribuito liberamente
 * secondo i termini delle licenze libere, GPL.
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>  // Include WiFi library
#include <credentials.h> // WIFI credentials for local wifi
#include <Wire.h>
#include <U8g2lib.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <display_manager.h>
#include <Arduino.h>     
#include <led_rgb_controller.h>

// SH1106, I2C, buffer pieno, senza pin reset
U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);
// Crea una iastanza di DisplayManager
DisplayManager screen(oled);

// Definisce il pin per ricevere dati di peso
#define SENSOR_PIN 32  // Use an ADC pin (GPIO32)

// Definisce il pin per il segnale del pulsante per la scansione QR
#define SCAN_PIN 4 // PIN per attivare scansione

//Definisce i pin per comunicazone UART
#define RXD2 16 // Serial2 RX from QR scanner
#define TXD2 17 // Serial2 TX from QR scanner

// Clinet wifi per connessione HTTPS
WiFiClientSecure client;
LedRGB led(13, 12, 14);  // R=13, G=12, B=14

// Url per le richieste JSON per ricevere dati prodotto
String apiUrl = "https://world.openfoodfacts.org/api/v0/product/";
//String apiUrl = "http://world.openfoodfacts.org/api/v0/product/";

int val;

void setup() {
  // Serial inizializzazione 115200 baud 
  Serial.begin(115200);  
  // Serial2 inizializzazione seriale per comunicare con lettore QR
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2); // Serial2 connect to GM65
  delay(200);


  // pulsante attivo alla pressione e pin led
  pinMode(SCAN_PIN, INPUT_PULLUP);
 

  // Imposta modalità comando per lettore GM65 QR code
  String mode = getWorkingMode();
  if (mode.indexOf("00210003") != -1) {
    Serial.println("✅ Modalità comando già attiva.");
  } else {
    Serial.println("⚙️ Imposto modalità comando...");
    Serial2.print("~M00210003.\r\n");
    delay(200);
    // salva come default
    Serial2.print("~MA5F0506A.\r\n"); 
    if (Serial2.available()) {
      Serial.println(Serial2.readStringUntil('\n')); // ti dirà [ACK] o [NAK]
    }
  }

  // inizializza display OLED
  screen.begin();                 
  screen.clearScreen();
  screen.setFont("Times", 10);
  screen.println("Init...");
  screen.println("Connessione WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // attendi la connessione WIFI per 10 secondi
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
  }
    led.blue();
    screen.clearScreen();
    screen.println("Connesso: ");
    screen.println(WiFi.SSID());
  
 
  Serial.println("Inizializzazione QR...");

  // Prova trigger
  Serial2.print("~T.\r\n");
  delay(500);  // attesa risposta

  // Prova la connessione a QR scanner
  if (Serial2.available()) {
    //String risposta = Serial2.readStringUntil('\n');
    screen.println("QR OK!");
    delay(500);
    screen.clearScreen();
  } else {
    screen.println("Errore QR!");
  }

  //led.test();

}


// Controlla modalità del modulo QR
String getWorkingMode() {
  Serial2.flush();
  Serial2.print("~Q0021.\r\n");
  delay(100);  // attesa breve

  String response = "";
  if (Serial2.available()) {
    response = Serial2.readStringUntil('\n');
  }

  return response;
}

// esegue una scansione con il moduleo GM65
// restituisce un codice numerico
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

      // Rimuove eventuali caratteri non numerici prima del vero codice
      int i = 0;
      while (i < raw.length() && !isDigit(raw[i])) i++;
      String codice = raw.substring(i);
      codice.trim();  

      return codice;
    }
  }

  return "";
}


// Connetti a OpenFoodFacts e raccogli informazioni del prodotto scansionato
void fetchProductData(String code) {
  if (WiFi.status() == WL_CONNECTED) {
    String url = apiUrl + code + ".json";
    Serial.println("➡️ URL: " + url);

    client.setInsecure();  // evita verifica certificato https
    HTTPClient http;
    http.begin(client, url);
    int httpCode = http.GET();

    if (httpCode == 200) {
      Serial.println("✅ HTTP OK → provo parsing JSON");

      String response = http.getString();
      Serial.println("📦 JSON ricevuto:");
      Serial.println(response);


      parseJSON(response);  // parsing diretto, ignorando Content-Type
    } else {
      Serial.print("❌ HTTP Code: ");
      Serial.println(httpCode);
      screen.clearScreen();
      screen.println("❌ Errore HTTP");
    }

    http.end();
  } else {
    Serial.println("⚠️ WiFi Disconnected!");
    screen.clearScreen();
    screen.println("❌ No WiFi");
  }
}

void loop() {
  //se il pulsante è premuto esegui una scansione
  if (digitalRead(SCAN_PIN) == HIGH) {
    screen.clearScreen();
    screen.println("Scan...");

    //richiama il codice per la scansione
    String code = scanQRCode();
    screen.println(code);
    if (code != "") {
      Serial.println("Codide letto: " + code);
      fetchProductData(code);
    } else {
      screen.println("NO CODE");
    }
    delay(500);
  }
  //Entra in low power mode se
  // - non c'è nulla appoggiato
  // - ho effettuato tutte le operazioni e il brikko è inserito 
  // 

  //Esci da low power mode se-
  // - sono attualmente in lpm ed è stato appoggiato un brikko
  // - nonè appoggiato un brikko ed è stato selezionato il bottone di scanQRCode


/*
int rawValue = analogRead(SENSOR_PIN);  // Read raw ADC value (0 - 4095)
  int weight = rawValue/100;
  // Write soething only it there is a chane in pressure
  if (rawValue != 0) {
    Serial.print("Peso: ");
    Serial.println(weight);
  }
*/  

  delay(500);  // Wait 500ms before next read
}


// Passa la stringa tornata dal server di openfoodfacts e la parserizza per ricavare 
// i dati dui cui abbiammo bisogno:
//  Nome prodotto
//  Nome produttore
//  Peso
//
void parseJSON(const String& jsonResponse) {
  DynamicJsonDocument doc(24576);  // 24 KB buffer per JSON grandi

  DeserializationError error = deserializeJson(doc, jsonResponse);
  if (error) {
    Serial.print("⚠️ JSON Parsing failed: ");
    Serial.println(error.c_str());
    screen.clearScreen();
    screen.println("❌ JSON error");
    return;
  }

  int status = doc["status"] | 0;
  if (status == 0) {
    Serial.println("❌ Prodotto non trovato");
    screen.clearScreen();
    screen.println("❌ NOT FOUND");
    return;
  }

  led.green();
  JsonObject product = doc["product"];
  String name = product["product_name"] | "Sconosciuto";
  String brand = product["brands"] | "Sconosciuto";
  String quantity = product["product_quantity"] | "N/D";

  Serial.println("🛒 Nome: " + name);
  Serial.println("🏭 Brand: " + brand);
  Serial.println("⚖️ Peso: " + quantity + "g");

  screen.clearScreen();
  screen.println(name);
  screen.println(brand);
  screen.println("Peso: " + quantity + "g");
}