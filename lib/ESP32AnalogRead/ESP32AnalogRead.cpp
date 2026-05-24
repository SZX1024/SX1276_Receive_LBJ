#include "ESP32AnalogRead.h"

ESP32AnalogRead::ESP32AnalogRead(int pin) : _pin(-1), _attached(false) {
    if (pin >= 0) attach(pin);
}

void ESP32AnalogRead::attach(int pin) {
    _pin = pin;
    _attached = true;
    pinMode(_pin, INPUT);
    analogReadResolution(12);
}

float ESP32AnalogRead::readVoltage() {
    return readMiliVolts() / 1000.0f;
}

uint32_t ESP32AnalogRead::readMiliVolts() {
    if (!_attached) return 0;
    return analogReadMilliVolts(_pin);
}

uint16_t ESP32AnalogRead::readRaw() {
    if (!_attached) return 0;
    return analogRead(_pin);
}
