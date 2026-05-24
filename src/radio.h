#pragma once

#include "config.h"

// SX1276 instances
extern SX1276 radio;
extern PagerClient pager;
extern const int pin;

// AFC / radio state
extern float rssi_cache;
extern float fers[32];
extern float actual_frequency;
extern float freq_last;
extern float car_fer_last;
extern float ppm;
extern bool freq_correction;
extern uint64_t prb_timer;
extern uint32_t prb_count;
extern uint64_t car_timer;
extern uint32_t car_count;
extern struct rx_info rxInfo;

// Telnet async relay flags
extern bool give_tel_rssi;
extern bool give_tel_gain;
extern bool tel_set_ppm;

// Functions
inline float actualFreq(float bias) {
    actual_frequency = (float) ((TARGET_FREQ * bias) / 1e6 + TARGET_FREQ);
    return actual_frequency;
}
int initPager();
void revertFrequency();
void handleCarrier();
void handlePreamble();
void handleSync();
float getBias(float freq);
