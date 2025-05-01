#ifndef LED_RGB_CONTROLLER_H
#define LED_RGB_CONTROLLER_H

#include <Arduino.h>

class LedRGB {
private:
  int redPin;
  int greenPin;
  int bluePin;

public:
  LedRGB(int rPin, int gPin, int bPin) {
    redPin = rPin;
    greenPin = gPin;
    bluePin = bPin;

    // Nuova API ledcAttach per ESP32 core v3.x
    ledcAttach(redPin, 5000, 8);
    ledcAttach(greenPin, 5000, 8);
    ledcAttach(bluePin, 5000, 8);
  }

  void setColor(uint8_t r, uint8_t g, uint8_t b) {
    ledcWrite(redPin, r);
    ledcWrite(greenPin, g);
    ledcWrite(bluePin, b);
  }

  void off() { setColor(0, 0, 0); }
  void red() { setColor(255, 0, 0); }
  void green() { setColor(0, 255, 0); }
  void blue() { setColor(0, 0, 255); }
  void yellow() { setColor(255, 255, 0); }
  void cyan() { setColor(0, 255, 255); }
  void magenta() { setColor(255, 0, 255); }
  void white() { setColor(255, 255, 255); }

  void test() {
    Serial.println("Testing Red LED");
    for (int val = 0; val < 255; val++) {
      ledcWrite(redPin, val);
      delay(25);
    }
    ledcWrite(redPin, 0);

    Serial.println("Testing Green LED");
    for (int val = 0; val < 255; val++) {
      ledcWrite(greenPin, val);
      delay(25);
    }
    ledcWrite(greenPin, 0);

    Serial.println("Testing Blue LED");
    for (int val = 0; val < 255; val++) {
      ledcWrite(bluePin, val);
      delay(25);
    }
    ledcWrite(bluePin, 0);

    Serial.println("Cycling all 3 LEDs");
    for (int val = 255; val > 0; val--) {
      ledcWrite(redPin, val);
      ledcWrite(bluePin, 255 - val);
      ledcWrite(greenPin, 128 - (val - 128));
      delay(25);
    }
    for (int val = 0; val < 255; val++) {
      ledcWrite(redPin, val);
      ledcWrite(bluePin, 255 - val);
      ledcWrite(greenPin, 128 - (val - 128));
      delay(25);
    }
    off();
  }
};

#endif
