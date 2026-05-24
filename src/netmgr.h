#pragma once

#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <ctime>
#include "esp_sntp.h"
#include <Preferences.h>
#include "ESPTelnet.h"

extern String wifiSSID;
extern String wifiPassword;

extern ESPTelnet telnet;
extern IPAddress ip;
extern uint16_t port;
extern bool is_startline;
extern bool no_wifi;
extern bool telnet_online;

extern const char *time_zone;
extern const char *ntpServer1;
extern const char *ntpServer2;
extern struct tm time_info;

bool isConnected();
bool connectToWiFi(const String &ssid, const String &password, int timeout);
void performSmartConfig();
void changeCpuFreq(uint32_t freq_mhz);
void timeAvailable(struct timeval *t);
void timeSync(struct tm &time);

void setupTelnet();
void onTelnetConnect(String ip);
void onTelnetDisconnect(String ip);
void onTelnetReconnect(String ip);
void onTelnetConnectionAttempt(String ip);
void onTelnetInput(String str);
void timeTask(void *pVoid);

void telPrintf(bool time_stamp, const char *format, ...);
void telPrintLog(int chars);

#ifdef HAS_RTC
#include <RTClib.h>
tm rtcLibtoC(const DateTime& datetime);
DateTime rtcLibtoC(const tm &ctime);
#endif
