#pragma once
#include <Arduino.h>

class ESP32AnalogRead {
public:
    ESP32AnalogRead(int pin = -1);
    void attach(int pin);
    float readVoltage();
    uint32_t readMiliVolts();
    uint16_t readRaw();
private:
    int _pin;
    bool _attached;
};
