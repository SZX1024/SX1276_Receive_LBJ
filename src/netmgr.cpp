#include "netmgr.h"
#include "radio.h"
#include "boards.hpp"
#include "sdlog.hpp"
#include <string>

/* ------------------------------------------------ */
Preferences preferences;
ESPTelnet telnet;
IPAddress ip;
uint16_t port = 23;

const char *time_zone = "CST-8";
const char *ntpServer1 = "pool.ntp.org";
const char *ntpServer2 = "time.nist.gov";

struct tm time_info{};

bool isConnected() {
    return (WiFi.status() == WL_CONNECTED);
}

void performSmartConfig() {
    WiFi.beginSmartConfig();
    Serial.println("[Network]Waiting for SmartConfig...");
    while (!WiFi.smartConfigDone()) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\n[Network]SmartConfig received.");
    Serial.println("[Network]Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\n[Network]WiFi connected.");
    preferences.putString("ssid", WiFi.SSID());
    preferences.putString("password", WiFi.psk());
    Serial.println("[Network]WiFi credentials saved.");
}

bool connectToWiFi(const String &ssid, const String &password, int timeout) {
    WiFi.begin(ssid.c_str(), password.c_str());
    Serial.print("Connecting to saved WiFi...");
    auto startAttemptTime = millis64();
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        if (millis64() - startAttemptTime > timeout)
        {
            Serial.println("\n[Network]Failed to connect.");
            return false;
        }
    }
    Serial.println("\n[Network]WiFi connected using saved credentials.");
    return true;
}

void changeCpuFreq(uint32_t freq_mhz) {
    if (isConnected() || no_wifi) {
        if (ets_get_cpu_frequency() != freq_mhz) {
            setCpuFrequencyMhz(freq_mhz);
            if (no_wifi) {
                WiFi.mode(WIFI_OFF);
                WiFi.setSleep(true);
            }
        }
    } else {
        auto timer = millis64();
        WiFi.mode(WIFI_OFF);
        Serial.printf("[D] WIFI OFF [%llu] \n", millis64() - timer);
        if (ets_get_cpu_frequency() != freq_mhz)
            setCpuFrequencyMhz(freq_mhz);
        WiFi.begin(wifiSSID, wifiPassword);
        WiFi.mode(WIFI_MODE_STA);
    }
}

/* ------------------------------------------------ */

void timeAvailable(struct timeval *t) {
    tm ti2{};
    Serial.println("[SNTP] Got time adjustment from NTP!");
    getLocalTime(&time_info);
    Serial.println(&time_info, "[SNTP] %Y-%m-%d %H:%M:%S");
#ifdef HAS_RTC
    getLocalTime(&time_info);
    rtc.adjust(rtcLibtoC(time_info));
    auto timer = esp_timer_get_time();
    ti2 = rtcLibtoC(rtc.now());
    Serial.print(&ti2, "[eRTC] Time set to %Y-%m-%d %H:%M:%S ");
    Serial.printf("[%lld]\n", esp_timer_get_time() - timer);
#endif
}

void timeSync(struct tm &time) {
    time_t t = mktime(&time);
    struct timeval now = {.tv_sec = t};
    settimeofday(&now, nullptr);
}

#ifdef HAS_RTC
tm rtcLibtoC(const DateTime &datetime) {
    tm time{};
    time.tm_year = datetime.year() - 1900;
    time.tm_mon = datetime.month() - 1;
    time.tm_mday = datetime.day();
    time.tm_wday = datetime.dayOfTheWeek();
    time.tm_yday = 0;
    time.tm_hour = datetime.hour();
    time.tm_min = datetime.minute();
    time.tm_sec = datetime.second();
    time.tm_isdst = 0;
    return time;
}

DateTime rtcLibtoC(const tm &ctime) {
    DateTime now(ctime.tm_year + 1900, ctime.tm_mon + 1, ctime.tm_mday, ctime.tm_hour, ctime.tm_min, ctime.tm_sec);
    return now;
}
#endif

/* ------------------------------------------------ */

void onTelnetConnect(String ip) {
    Serial.print("[Telnet] ");
    Serial.print(ip);
    Serial.println(" connected");

    telnet.println("===[ESP32 DEV MODULE TELNET SERVICE]===");
    getLocalTime(&time_info, 10);
    char timeStr[20];
    sprintf(timeStr, "%d-%02d-%02d %02d:%02d:%02d", time_info.tm_year + 1900, time_info.tm_mon + 1, time_info.tm_mday,
            time_info.tm_hour, time_info.tm_min, time_info.tm_sec);
    telnet.print("System time is ");
    telnet.print(timeStr);
    telnet.println("\n\rWelcome " + telnet.getIP());
    telnet.println("(Use ^] + q  to disconnect.)");
    telnet.println("=======================================");
    telnet.print("< ");
}

void onTelnetDisconnect(String ip) {
    Serial.print("[Telnet] ");
    Serial.print(ip);
    Serial.println(" disconnected");
}

void onTelnetReconnect(String ip) {
    Serial.print("[Telnet] ");
    Serial.print(ip);
    Serial.println(" reconnected");
}

void onTelnetConnectionAttempt(String ip) {
    Serial.print("[Telnet] ");
    Serial.print(ip);
    Serial.println(" tried to connected");
}

void onTelnetInput(String str) {
    bool ext_call = false;
    size_t args_count = 3;
    char strc[256];
    str.toCharArray(strc, 256);
    String args[3];
    bool toolong = false;
    for (size_t i = 0, c = 0; i < 256; i++) {
        if (strc[i] == 0)
            break;
        if (strc[i] == ' ') {
            i++;
            c++;
        }
        if (c > args_count - 1) {
            toolong = true;
            break;
        }
        args[c] += strc[i];
    }
    if (!toolong) {
        if (args[0] == "test") {
            if (args[1] == "test") {
                telnet.println("> Args test.");
            } else
                telnet.println("> Unknown Command.");
        }
        if (args[0] == "log") {
            if (args[1] == "read") {
                bool valid = true;
                if (args[2].length() == 0)
                    valid = false;
                for (auto c: args[2]) {
                    if (!isDigit(c))
                        valid = false;
                }
                if (!valid)
                    telnet.println("> Invalid Format, digits only.");
                else {
                    if (sd1.status())
                        sd1.printTel(std::stoi(args[2].c_str()), telnet);
                    else
                        telnet.println("> Can not access SD card.");
                }
            } else if (args[1] == "status") {
                if (sd1.status())
                    telnet.println("> True.");
                else
                    telnet.println("> False.");
            } else
                telnet.println("> Unknown Command.");
        }
        if (args[0] == "afc") {
            if (args[1] == "off") {
                prb_count = 0;
                prb_timer = 0;
                freq_correction = false;
                telnet.println("> Frequency Correction Disabled");
                Serial.println("[Telnet] > Frequency Correction Disabled");
            }
            else if (args[1] == "on") {
                freq_correction = true;
                telnet.println("> Frequency Correction Enabled");
                Serial.println("[Telnet] > Frequency Correction Enabled");
            }
        }
        if (args[0] == "ppm") {
            if (args[1].length() != 0 ){
                bool valid = true;
                bool point = false;
                for (auto c:args[1]){
                    if (!isDigit(c)) {
                        if (c == '.'&& !point)
                            point = true;
                        else
                            valid = false;
                    }
                }
                if (valid) {
                    ppm = std::stof(args[1].c_str());
                    tel_set_ppm = true;
                } else
                    telnet.println("> Invalid Format, float only.");
            }
        }
    }

    if (str == "ping") {
        telnet.println("> pong");
        Serial.println("[Telnet] > pong");
    } else if (str == "bye") {
        telnet.println("> disconnecting you...");
        telnet.disconnectClient();
    } else if (str == "read") {
        if (sd1.status())
            telPrintLog(1000);
        else
            telnet.println("> Can not access SD card.");
    } else if (str == "bat")
        telnet.printf("Current Battery Voltage %1.2fV\n", battery.readVoltage() * 2);
    else if (str == "rssi") {
        give_tel_rssi = true;
        ext_call = true;
    } else if (str == "gain") {
        give_tel_gain = true;
        ext_call = true;
    } else if (str == "time") {
        telPrintf(true, "Time requested.\n");
        Serial.printf("[Telnet] > Time requested.\n");
        ext_call = true;
    } else if (str == "task time") {
        xTaskCreatePinnedToCore(timeTask, "timeTask", 2048,
                                nullptr, 1, nullptr, ARDUINO_RUNNING_CORE);
        Serial.println("[Telnet] > Thread time requested.");
    }
    if (!ext_call)
        telnet.print("< ");
}

/* ------------------------------------------------- */

void setupTelnet() {
    telnet.onConnect(onTelnetConnect);
    telnet.onConnectionAttempt(onTelnetConnectionAttempt);
    telnet.onReconnect(onTelnetReconnect);
    telnet.onDisconnect(onTelnetDisconnect);
    telnet.onInputReceived(onTelnetInput);

    Serial.print("[Telnet] ");
    if (telnet.begin(port)) {
        Serial.println("running");
        telnet_online = true;
    } else {
        Serial.println("error.");
    }
}

/* ------------------------------------------------- */

void timeTask(void *pVoid) {
    telPrintf(true, "Thread time requested.\n");
    delay(5000);
    Serial.printf("Running on Core %d\n", xPortGetCoreID());
    telPrintf(true, "Thread time requested.\n");
    Serial.println("Thread time requested.");
    vTaskDelete(nullptr);
}

/* ------------------------------------------------- */

void telPrintf(bool time_stamp, const char *format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (telnet_online) {
        if (is_startline) {
            telnet.print("\r> ");
            if (time_stamp && getLocalTime(&time_info, 1))
                telnet.printf("\r%d-%02d-%02d %02d:%02d:%02d > ", time_info.tm_year + 1900, time_info.tm_mon + 1,
                              time_info.tm_mday, time_info.tm_hour, time_info.tm_min, time_info.tm_sec);
            is_startline = false;
        }
        telnet.print(buffer);
        if (nullptr != strchr(buffer, '\n')) {
            is_startline = true;
            telnet.print("\r< ");
        }
    }
}

void telPrintLog(int chars) {
    File log = sd1.logFile('r');
    uint32_t pos, left;
    if (chars < log.size())
        pos = log.size() - chars;
    else
        pos = 0;
    left = chars - log.size();
    String line;
    if (!log.seek(pos))
        Serial.println("[SDLOG] seek failed!");
    while (log.available()) {
        line = log.readStringUntil('\n');
        if (line) {
            telnet.print(line);
            telnet.print("\n");
        } else
            telPrintf(false, "[SDLOG] Read failed!");
    }
    if (left)
        sd1.reopen();
}
