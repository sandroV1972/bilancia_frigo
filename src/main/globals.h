#pragma once
#include <Preferences.h>
#include <Arduino.h>

extern Preferences prefs;

// Parametri namespace del prodotto in memoria
// nome       | type (default)  | descrizione
// _________________________________________________________________________
// prodotto.  - String ().       - nome del prodotto
// marca.     - String ().       - marca del prodotto
// peso.      - float (0.0).     - peso del prodotto pieno
// ssid       - String ()        - ssid dell'ultima rete a cui sei connesso
// passowrd   - String.          - password rete wifi 
//
extern String prodName;
extern String prodBrand;
extern float prodWeight;
extern const String ssid_captive;
extern String ssid;
extern String password;
extern bool inviamqtt;

// 