#ifndef LED_RGB_CONTROLLER_H
#define LED_RGB_CONTROLLER_H

#include <Arduino.h>

class LedRGB {
private:
  int redPin;
  int greenPin;
  int bluePin;
  float intensity = 1.0;  // Intensità da 0.0 a 1.0

public:
  LedRGB(int rPin, int gPin, int bPin) {
    redPin = rPin;
    greenPin = gPin;
    bluePin = bPin;

    // Attach canali PWM con frequenza e risoluzione predefinita (ESP32 core v3.x)
    ledcAttach(redPin, 5000, 8);
    ledcAttach(greenPin, 5000, 8);
    ledcAttach(bluePin, 5000, 8);
  }

  void setIntensity(float value) {
    intensity = constrain(value, 0.0, 1.0);
  }

  void setColor(uint8_t r, uint8_t g, uint8_t b) {
    ledcWrite(redPin, r * intensity);
    ledcWrite(greenPin, g * intensity);
    ledcWrite(bluePin, b * intensity);
  }

  void off() { setColor(0, 0, 0); }
  void red() { setColor(255, 0, 0); }
  void green() { setColor(0, 255, 0); }
  void blue() { setColor(0, 0, 255); }
  void yellow() { setColor(255, 255, 0); }
  void cyan() { setColor(0, 255, 255); }
  void magenta() { setColor(255, 0, 255); }
  void white() { setColor(255, 255, 255); }
};

#endif