#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "id_bilancia.h"
#include "globals.h"
#include <WiFi.h>
#include <Preferences.h>
#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager


// Verifica se una rete salvata è presente tra quelle disponibili
bool wifiReady() {
  prefs.begin("settings", true);  // modalità lettura
  ssid = prefs.getString("ssid", "");
  password = prefs.getString("password", "");
  Serial.println("SSID: " + ssid);
  prefs.end();

  if (ssid == "") {
    return false;  // Nessuna credenziale salvata
  }

  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; ++i) {
    if (WiFi.SSID(i) == ssid) {
      // Rete trovata
      //WiFi.begin(ssid.c_str(), password.c_str());
      //if (WiFi.waitForConnectResult() == WL_CONNECTED) {
        WiFi.mode(WIFI_MODE_NULL);  // disattiva WiFi per risparmio
        return true;
      //}
    }
  }

  WiFi.mode(WIFI_MODE_NULL);
  return false;
}

// Inizializza la connessione WiFi tramite WiFiManager e salva le credenziali
void initWifi() {
  WiFiManager wm;

  wm.setConnectTimeout(30);  // timeout captive portal
  bool res = wm.autoConnect(ssid_captive.c_str());

  if (res) {
    String this_ssid = WiFi.SSID();
    String this_pass = WiFi.psk();

    prefs.begin("settings", false);  // modalità scrittura
    prefs.putString("ssid", this_ssid);
    prefs.putString("password", this_pass);
    prefs.end();
  }

  WiFi.mode(WIFI_MODE_NULL);  // spegne WiFi dopo setup
}
 bool connectWiFi() {
    prefs.begin("settings", true);  // true = solo lettura
    String ssid = prefs.getString("ssid", "");
    String password = prefs.getString("password", "");
    prefs.end();

    if (ssid.isEmpty() || password.isEmpty()) {
      Serial.println("❌ Nessuna credenziale salvata in NVS.");
      return false;
    }
    WiFi.mode(WIFI_STA);
    IPAddress dns(8,8,8,8);
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, dns);
    WiFi.begin(ssid.c_str(), password.c_str());

    Serial.print("🔌 Connessione a WiFi: ");
    Serial.print(ssid);

    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
      delay(250);
      Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("✅ WiFi connesso con IP: " + WiFi.localIP().toString());
      Serial.print("DNS configurato: ");
      Serial.println(WiFi.dnsIP());
      return true;
    } else {
      Serial.println("❌ Connessione WiFi fallita");
      return false;
    }
  }

  void disconnectWiFi() {
    WiFi.disconnect(true);  // true = cancella anche le credenziali dalla RAM
    WiFi.mode(WIFI_OFF);    // spegne fisicamente il modulo radio
    delay(100);             // stabilizza
    Serial.println("📴 WiFi disconnesso e spento.");
  }


#endif