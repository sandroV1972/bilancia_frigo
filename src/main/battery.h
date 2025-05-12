// battery.h
#ifndef BATTERY_H
#define BATTERY_H

#include <Arduino.h>

class Battery {
  private:
  int battPin;
  float R1, R2;
  
  public:

  // Costruttore con pin e valori partitore
  Battery(int pin, float r1 = 100000.0, float r2 = 100000.0)
    : battPin(pin), R1(r1), R2(r2) {}


  // Calcola la tensione reale della batteria
float leggiTensioneBatteria() {
    int adc = analogRead(this->battPin); // usa il campo membro
    float voltage = (adc / 4095.0) * 3.3;
    float v_batt = voltage * ((this->R1 + this->R2) / this->R2) * (3.56/3.18);
    return v_batt;
}

  // Converte la tensione della batteria in percentuale (curva non lineare)
  int stimaCaricaBatteria(float v_batt) {
    if (v_batt >= 4.20) return 100;
    else if (v_batt >= 4.15) return 95;
    else if (v_batt >= 4.11) return 90;
    else if (v_batt >= 4.08) return 85;
    else if (v_batt >= 4.02) return 80;
    else if (v_batt >= 3.98) return 75;
    else if (v_batt >= 3.95) return 70;
    else if (v_batt >= 3.91) return 65;
    else if (v_batt >= 3.87) return 60;
    else if (v_batt >= 3.84) return 55;
    else if (v_batt >= 3.80) return 50;
    else if (v_batt >= 3.77) return 45;
    else if (v_batt >= 3.73) return 40;
    else if (v_batt >= 3.70) return 35;
    else if (v_batt >= 3.67) return 30;
    else if (v_batt >= 3.61) return 25;
    else if (v_batt >= 3.55) return 20;
    else if (v_batt >= 3.50) return 15;
    else if (v_batt >= 3.44) return 10;
    else if (v_batt >= 3.39) return 5;
    else return 0;
  }

  // Funzione unica per ottenere la % di batteria
  int percentualeBatteria() {
    float v = leggiTensioneBatteria();
    return stimaCaricaBatteria(v);
  }

};
#endif
