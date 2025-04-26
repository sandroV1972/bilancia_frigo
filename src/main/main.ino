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

#include <WiFi.h>  // Include WiFi library
#include <credentals.h> // WIFI credentials for local wifi

#define SENSOR_PIN 32  // Use an ADC pin (GPIO32)

String barcode = "000000000000";  // Coca-Cola example
String apiUrl = "https://world.openfoodfacts.org/api/v0/product/";

Preferences preferences; 

void setup() {
  Serial.begin(115200);  // Start serial communication
  
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);  // Connect to WiFi

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // 🔹 Open non-volatile storage (namespace: "storage")
  preferences.begin("storage", false);

}

void loop() {
  //Entra in low power mode se
  // - non c'è nulla appoggiato
  // - ho effettuato tutte le operazioni e il brikko è inserito 
  // 

  //Esci da low power mode se-
  // - sono attualmente in lpm ed è stato appoggiato un brikko
  // - nonè appoggiato un brikko ed è stato selezionato il bottone di scanQRCode


  int rawValue = analogRead(SENSOR_PIN);  // Read raw ADC value (0 - 4095)
  int weight = rewValue/100;
  // Write soething only it there is a chane in pressure
  if (rawValue != 0) {
    Serial.print("Peso: ");
    Serial.print(weight);


  }

  //Verifica se è stato selezionato il bottone di lettura
  //Fai la scan solo se nulla è appogiato all apiattaforma
  // scanQRCode();



  
  delay(500);  // Wait 500ms before next read
}

//Funzioni di scansione del codice a barre

// Connetti a OpenFoodFacts e raccogli informazioni del prodotto scansionato
void fetchProductData(inr barcode) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(apiUrl+ barcode + ".json");  // Start connection
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String jsonResponse = http.getString();
      Serial.println("✅ JSON Data Received:");
      Serial.println(jsonResponse);

      parseJSON(jsonResponse);  // Parse JSON response
    } else {
      Serial.print("❌ Error in HTTP request. Code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("⚠️ WiFi Disconnected!");
  }
}

// Parse JSON file fro the Data needed
void parseJSON(String jsonResponse) {
  // Create a JSON document
  DynamicJsonDocument doc(8192);  // Use a large enough buffer

  DeserializationError error = deserializeJson(doc, jsonResponse);
  if (error) {
    Serial.print("⚠️ JSON Parsing failed: ");
    Serial.println(error.c_str());
    return;
  }

  // Extract Product Data
  String name = doc["product"]["product_name"] | "Unknown";
  String brand = doc["product"]["brands"] | "Unknown";
  String image = doc["product"]["image_url"] | "No image available";

  // Print Extracted Data
  Serial.println("\n📌 Product Info:");
  Serial.println("🛒 Name: " + name);
  Serial.println("🏭 Brand: " + brand);
  
  Serial.println("📷 Image URL: " + image);
}

