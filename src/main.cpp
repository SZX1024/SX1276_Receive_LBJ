/*
   SX1276 LBJ Message Receive Project
   Migrated by FLN1021 on Sep 2023.
   Modified from RadioLib Pager (POCSAG) Receive Example.
*/

#pragma execution_character_set("utf-8")

#include <RadioLib.h>
#include "radio.h"
#include "netmgr.h"
#include "output.h"
#include "display.h"
#include "protocol.h"
#include "coredump.h"
#include "customfont.h"
#include "sdlog.hpp"
#include "boards.hpp"
#include "utils.h"
#include <esp_task_wdt.h>

// network state (defined here, extern'd in network.h)
String wifiSSID = "801";
String wifiPassword = "zmsyhmmc";
bool no_wifi = false;
bool telnet_online = false;
bool is_startline = true;

// application state
uint64_t format_task_timer = 0;
uint64_t runtime_timer = 0;
uint64_t led_timer = 0;
uint64_t net_timer = 0;
uint32_t ip_last = 0;
bool exec_init_f80 = false;
bool have_cd = false;
struct data_bond *db = nullptr;

TaskHandle_t task_fd;
task_states fd_state;

// forward declarations
void formatDataTask(void *pVoid);
void simpleFormatTask();
void initFmtVars();
void handleSerialInput();

void dualPrintf(bool time_stamp, const char *format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    Serial.print(buffer);

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

void dualPrint(const char *fmt) {
    Serial.print(fmt);
    telnet.print(fmt);
}

void dualPrintln(const char *fmt) {
    Serial.println(fmt);
    telnet.println(fmt);
}

// SETUP
void setup() {
    esp_core_dump_init();
    runtime_timer = millis64();
    esp_reset_reason_t reset_reason = esp_reset_reason();
    initBoard();
    sd1.setFS(SD);
    delay(150);

    sntp_set_time_sync_notification_cb(timeAvailable);
    configTzTime(time_zone, ntpServer1, ntpServer2);

#ifdef HAS_RTC
    time_info = rtcLibtoC(rtc.now());
    Serial.println(&time_info, "[eRTC] RTC Time %Y-%m-%d %H:%M:%S ");
    timeSync(time_info);
    Serial.printf("SYS Time %s\n", fmtime(time_info));
#endif

    Serial.printf("RST: %s\n", printResetReason(reset_reason).c_str());
    if (have_sd) {
        sd1.begin("/LOGTEST");
        sd1.beginCSV("/CSVTEST");
        sd1.append("电池电压 %1.2fV\n", battery.readVoltage() * 2);
        sd1.append(2, "调试等级 %d\n", LOG_VERBOSITY);
        sd1.append("复位原因 %s\n", printResetReason(reset_reason).c_str());
#ifdef HAS_RTC
        sd1.append("RTC时间 %d-%02d-%02d %02d:%02d:%02d\n", time_info.tm_year + 1900, time_info.tm_mon + 1,
                   time_info.tm_mday, time_info.tm_hour, time_info.tm_min, time_info.tm_sec);
#endif
    }

    readCoreDump();

    if (u8g2) {
        showInitComp();
        u8g2->setFont(FONT_12_GB2312);
        u8g2->setCursor(0, 52);
        u8g2->println("Initializing...");
        u8g2->sendBuffer();
    }

#ifdef USE_SMARTCONFIG
    Serial.printf("Connecting to WiFi\n");

    Preferences preferences;
    preferences.begin("wifi-config", false);

    String savedSSID = preferences.getString("ssid", "");
    String savedPassword = preferences.getString("password", "");

    if (!savedSSID.isEmpty() && !savedPassword.isEmpty()) {
        if (u8g2) {
            u8g2->setDrawColor(0);
            u8g2->drawBox(0, 42, 128, 14);
            u8g2->setDrawColor(1);
            u8g2->setCursor(0, 52);
            u8g2->println("Waiting for WiFi...");
            u8g2->sendBuffer();
        }
        if (!connectToWiFi(savedSSID, savedPassword, 10000)) {
            if (u8g2) {
                u8g2->setDrawColor(0);
                u8g2->drawBox(0, 42, 128, 14);
                u8g2->setDrawColor(1);
                u8g2->setCursor(0, 40);
                u8g2->println("Failed to connect to Wifi");
                u8g2->setCursor(0, 52);
                u8g2->println("Waiting for SmartConfig...");
                u8g2->sendBuffer();
            }
            performSmartConfig();
        }
    } else {
        if (u8g2) {
            u8g2->setDrawColor(0);
            u8g2->drawBox(0, 42, 128, 14);
            u8g2->setDrawColor(1);
            u8g2->setCursor(0, 52);
            u8g2->println("Waiting for SmartConfig...");
            u8g2->sendBuffer();
        }
        performSmartConfig();
    }

    Serial.print("[Network]IP Address: ");
    Serial.println(WiFi.localIP());
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    preferences.putString("ssid", WiFi.SSID());
    preferences.putString("password", WiFi.psk());
    preferences.end();
    wifiSSID = WiFi.SSID();
    wifiPassword = WiFi.psk();
#else
    Serial.printf("Connecting to WiFi %s\n", wifiSSID.c_str());
    if (u8g2) {
        u8g2->setDrawColor(0);
        u8g2->drawBox(0, 42, 128, 14);
        u8g2->setDrawColor(1);
        u8g2->drawStr(0, 52, "Connecting to WiFi...");
        u8g2->sendBuffer();
    }
    connectToWiFi(wifiSSID, wifiPassword, 1000);
#endif

    if (isConnected()) {
        ip = WiFi.localIP();
        esp_sntp_servermode_dhcp(1);
        Serial.print("[Telnet] ");
        Serial.print(ip);
        Serial.print(":");
        Serial.println(port);
        setupTelnet();
    } else {
        Serial.println("Error connecting to WiFi, Telnet startup skipped.");
    }

    dualPrint("[SX1276] Initializing ... ");
    int state = initPager();
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("success."));
        Serial.printf("[SX1276] Actual Frequency %f MHz, ppm %.1f\n", actualFreq(ppm), ppm);
    } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
        while (true);
    }

    esp_task_wdt_config_t wdt_config = {
        .timeout_ms = WDT_TIMEOUT * 1000,
    };
    esp_task_wdt_init(&wdt_config);
    esp_task_wdt_add(nullptr);

    digitalWrite(BOARD_LED, LED_OFF);
    Serial.printf("Booting time %llu ms\n", millis64() - runtime_timer);
    sd1.append("启动用时 %llu ms\n", millis64() - runtime_timer);
    runtime_timer = 0;

    if (u8g2) {
        u8g2->setDrawColor(0);
        u8g2->drawBox(0, 42, 128, 14);
        u8g2->setDrawColor(1);
        u8g2->drawStr(0, 52, "Listening...");
        u8g2->sendBuffer();
        Serial.printf("Mem left: %d Bytes\n", esp_get_free_heap_size());
    }
}

void handleTelnetCall() {
    if (give_tel_rssi) {
        telnet.printf("> RSSI %3.2f dBm.\n", radio.getRSSI(false, true));
        give_tel_rssi = false;
        telnet.print("< ");
    }
    if (give_tel_gain) {
        telnet.printf("> Gain Pos %d \n", radio.getGain());
        give_tel_gain = false;
        telnet.print("< ");
    }
    if (tel_set_ppm) {
        int16_t state = radio.setFrequency(actualFreq(ppm));
        if (state == RADIOLIB_ERR_NONE) {
            telnet.printf("> Actual Frequency %f MHz\n", actualFreq(ppm));
            Serial.printf("[Telnet] > Actual Frequency %f MHz\n", actualFreq(ppm));
        } else {
            telnet.printf("> Failure, Code %d\n", state);
            Serial.printf("[Telnet] > Failure, Code %d\n", state);
        }
        telnet.printf("> ppm set to %.f\n", ppm);
        tel_set_ppm = false;
        telnet.print("< ");
    }
}

void handleTelnet() {
    if (isConnected() && !telnet_online) {
        ip = WiFi.localIP();
        Serial.printf("WIFI Connection to %s established.\n", wifiSSID.c_str());
        Serial.print("[Telnet] ");
        Serial.print(ip);
        Serial.print(":");
        Serial.println(port);
        setupTelnet();
    }
    telnet.loop();
}

void checkNetwork() {
    if (isConnected() && net_timer != 0)
        net_timer = 0;
    else if (!isConnected() && net_timer == 0)
        net_timer = millis64();

    if (!isConnected() && millis64() - net_timer > NETWORK_TIMEOUT && !no_wifi) {
        telnet.stop();
        telnet_online = false;
        WiFi.disconnect();
        WiFi.mode(WIFI_OFF);
        Serial.println("WIFI off after 30 minutes without connection.");
        no_wifi = true;
    }

    if (ip_last != WiFi.localIP()) {
        Serial.print("Local IP ");
        Serial.print(WiFi.localIP());
        Serial.print("\n");
    }
    ip_last = WiFi.localIP();
}

// LOOP
void loop() {
    esp_task_wdt_reset();

    if (car_timer != 0 && millis64() - car_timer > 700 && prb_timer == 0 && rxInfo.timer == 0) {
        car_count = 0;
        revertFrequency();
        car_fer_last = 0;
        car_timer = 0;
        Serial.println("[D] CARRIER TIMEOUT.");
    }

    if (prb_timer != 0 && millis64() - prb_timer > 600 && rxInfo.timer == 0) {
        prb_count = 0;
        revertFrequency();
        for (auto &i: fers) {
            i = 0;
        }
        prb_timer = 0;
        Serial.println("[D] PREAMBLE TIMEOUT.");
    }

    if (fd_state == TASK_DONE) {
        if (task_fd != nullptr) {
            vTaskDelete(task_fd);
            task_fd = nullptr;
        }
        initFmtVars();
        fd_state = TASK_INIT;
        format_task_timer = 0;
    } else if (fd_state == TASK_CREATE_FAILED) {
        initFmtVars();
        format_task_timer = 0;
        fd_state = TASK_INIT;
    }

    if (millis64() - led_timer > LED_ON_TIME && led_timer != 0 && fd_state == TASK_INIT) {
        digitalWrite(BOARD_LED, LED_OFF);
        led_timer = 0;
        changeCpuFreq(240);
    }

    handleSerialInput();
    checkNetwork();
    handleTelnet();
    handleTelnetCall();

    if (millis64() > 60000 && format_task_timer == 0 && !exec_init_f80) {
        if (isConnected())
            setCpuFrequencyMhz(80);
        else {
            WiFi.mode(WIFI_OFF);
            setCpuFrequencyMhz(80);
            WiFi.mode(WIFI_MODE_STA);
            WiFi.begin(wifiSSID, wifiPassword);
        }
        exec_init_f80 = true;
    }

#ifdef HAS_DISPLAY
    if (screen_timer == 0) {
        screen_timer = millis64();
    } else if (millis64() - screen_timer > 3000) {
#ifdef HAS_OLED_TIMEOUT
        if (!oled_off)
#endif
            updateInfo(actual_frequency);
        screen_timer = millis64();
    }
#ifdef HAS_OLED_TIMEOUT
    if (millis64() - timer4 >= OLED_TIMEOUT && timer4 != 0 && !oled_off) {
        u8g2->clearBuffer();
        oled_off = true;
        u8g2->setPowerSave(true);
    }
#endif
#endif

    if (millis64() - format_task_timer >= FD_TASK_TIMEOUT && (fd_state == TASK_RUNNING || fd_state == TASK_CREATED)
        && task_fd != nullptr && format_task_timer != 0) {
        vTaskDelete(task_fd);
        task_fd = nullptr;
        dualPrintln("[Pager] FD_TASK Timeout.");
        sd1.append("[Pager] FD_TASK Timeout.\n");
        initFmtVars();
        Serial.printf("LED LOW [%llu]\n", millis64() - format_task_timer);
        digitalWrite(BOARD_LED, LED_OFF);
        format_task_timer = 0;
        led_timer = 0;
        changeCpuFreq(240);
        fd_state = TASK_INIT;
    }

    if (millis64() - timer4 >= 60000 && timer4 != 0 && ets_get_cpu_frequency() != 80)
        changeCpuFreq(80);

    handleCarrier();
    handlePreamble();
    handleSync();

    if (pager.available() >= 2 && fd_state == TASK_INIT) {
        setCpuFrequencyMhz(240);
        db = new data_bond;
        runtime_timer = millis64();
        timer4 = millis64();
        int state = pager.readDataMSA(db->pocsagData, 0);
        rxInfo.rssi = rssi_cache / (float) rxInfo.cnt;
        rssi_cache = 0;
        rxInfo.cnt = 0;
        rxInfo.timer = 0;
        prb_timer = 0;
        car_timer = 0;

        Serial.printf("[D] Prb_count %d\n", prb_count);
        Serial.printf("[D] Car_count %d\n", car_count);
        if (prb_count >= 32)
            prb_count = 31;
        if (prb_count > 0)
            rxInfo.fer = fers[prb_count - 1];
        for (int i = 0; i < prb_count; ++i) {
            Serial.printf("[D] Fer %.2f Hz\n", fers[i]);
            fers[i] = 0;
        }
        prb_count = 0;
        car_count = 0;
        car_fer_last = 0;
        rxInfo.ppm = getBias(actual_frequency);

        Serial.println(F("[Pager] Received pager data, decoding ... "));
        sd1.append(2, "正在解码信号...\n");

        if (state == RADIOLIB_ERR_NONE) {
            freq_last = actual_frequency;
            digitalWrite(BOARD_LED, LED_ON);
            format_task_timer = millis64();
            led_timer = millis64();

            sd1.append(2, "正在格式化输出...\n");
            auto x_ret = xTaskCreatePinnedToCore(formatDataTask, "task_fd",
                                                 FD_TASK_STACK_SIZE, nullptr,
                                                 2, &task_fd, ARDUINO_RUNNING_CORE);
            if (x_ret == pdPASS) {
                fd_state = TASK_CREATED;
                delay(1);
            } else if (x_ret == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY) {
                int x_ret1;
                for (int i = 0; i < FD_TASK_ATTEMPTS; ++i) {
                    x_ret1 = xTaskCreatePinnedToCore(formatDataTask, "task_fd",
                                                     FD_TASK_STACK_SIZE, nullptr,
                                                     2, &task_fd, ARDUINO_RUNNING_CORE);
                    if (x_ret1 == pdPASS) {
                        fd_state = TASK_CREATED;
                        delay(1);
                        break;
                    }
                    Serial.printf("[Pager] FTask failed memory allocation, error %d, mem left %d B, retry %d\n",
                                  x_ret1, esp_get_free_heap_size(), i);
                    sd1.append("[Pager] FTask failed memory allocation, error %d, mem left %d B, retry %d\n",
                               x_ret1, esp_get_free_heap_size(), i);
                }
                if (x_ret1 != pdPASS) {
                    Serial.printf("Mem left: %d Bytes\n", esp_get_free_heap_size());
                    dualPrintf(true, "[Pager] Format task memory allocation failure\n");
                    sd1.append("[Pager] Format task memory allocation failure, Mem left %d Bytes\n",
                               esp_get_free_heap_size());
                    fd_state = TASK_CREATE_FAILED;
                    simpleFormatTask();
                    digitalWrite(BOARD_LED, LED_OFF);
                }
            } else {
                dualPrintf(true, "[Pager] Failed to create format task, errcode %d\n", x_ret);
                sd1.append("[Pager] Failed to create format task, errcode %d\n", x_ret);
                fd_state = TASK_CREATE_FAILED;
                digitalWrite(BOARD_LED, LED_OFF);
            }

        } else if (state == RADIOLIB_ERR_MSG_CORRUPT) {
            dualPrintf(true, "[Pager] Reception failed, too many errors. \n");
            revertFrequency();
        } else {
            sd1.append("[Pager] Reception failed, code %d \n", state);
            dualPrintf(true, "[Pager] Reception failed, code %d \n", state);
        }

        if (fd_state == TASK_INIT) {
            initFmtVars();
        } else if (fd_state == TASK_CREATED && task_fd == nullptr) {
            fd_state = TASK_DONE;
        }
    }
}

void getCoreFreq(void *pVoid) {
    Serial.printf("Core %d Frequency %d MHz\n", xPortGetCoreID(), ets_get_cpu_frequency());
    vTaskDelete(nullptr);
}

void handleSerialInput() {
    if (Serial.available()) {
        String in = Serial.readStringUntil('\r');
        if (in == "ping")
            Serial.println("$ Pong");
        else if (in == "task state")
            Serial.println("$ Task state " + String(fd_state));
        else if (in == "rtc") {
#ifdef HAS_RTC
            time_info = rtcLibtoC(rtc.now());
            float temp = rtc.getTemperature();
            Serial.print(&time_info, "$ [eRTC] %Y-%m-%d %H:%M:%S ");
            Serial.printf("Temp: %.2f °C\n", temp);
#endif
        } else if (in == "time") {
            getLocalTime(&time_info, 1);
            Serial.printf("$ SYS Time %s, Up time %llu ms (%s)\n", fmtime(time_info), millis64(), fmtms(millis64()));
        } else if (in == "cd") {
            if (have_cd)
                Serial.println("$ Core dump exported.");
            else
                Serial.println("$ No core dump.");
        } else if (in == "sd end") {
            if (!sd1.status())
                Serial.println("$ [SDLOG] No SD.");
            else {
                sd1.append("[SDLOG] SD卡将被卸载\n");
                sd1.end();
                Serial.println("$ [SDLOG] SD end.");
            }
        } else if (in == "sd begin") {
            if (sd1.status())
                Serial.println("$ End SD First.");
            else {
                SD_LOG::reopenSD();
                sd1.begin("/LOGTEST");
                sd1.beginCSV("/CSVTEST");
                sd1.append("[SDLOG] SD卡已重新挂载\n");
                Serial.println("$ [SDLOG] SD reopen.");
            }
        } else if (in == "mem") {
            Serial.printf("$ Mem left: %d Bytes\n", esp_get_free_heap_size());
        } else if (in == "rst") {
            esp_reset_reason_t reason = esp_reset_reason();
            Serial.printf("$ RST: %s\n", printResetReason(reason).c_str());
        } else if (in == "ppm") {
            if (runtime_timer == 0 && !pager.gotSyncState()) {
                ppm = 3;
                int16_t state = radio.setFrequency(actualFreq(ppm));
                if (state == RADIOLIB_ERR_NONE)
                    Serial.printf("$ Actual Frequency %f MHz\n", actualFreq(ppm));
                else
                    Serial.printf("$ Failure, Code %d\n", state);
            } else {
                Serial.println("$ Unable to change frequency due to occupation");
                if (pager.available())
                    Serial.println("$ pager.available == true");
                if (runtime_timer)
                    Serial.printf("$ runtime_timer = %llu, running %llu\n", runtime_timer, millis64() - runtime_timer);
            }
        } else if (in == "ppm read") {
            Serial.printf("$ ppm %.1f\n", ppm);
        } else if (in == "afc off") {
            prb_count = 0;
            prb_timer = 0;
            car_count = 0;
            car_timer = 0;
            freq_correction = false;
            Serial.println("$ Frequency Correction Disabled");
        } else if (in == "afc on") {
            freq_correction = true;
            Serial.println("$ Frequency Correction Enabled");
        } else if (in == "rssi") {
            Serial.printf("$ RSSI %3.2f dBm.\n", radio.getRSSI(false, true));
        } else if (in == "gain") {
            Serial.printf("$ Gain Pos %d \n", radio.getGain());
        } else if (in == "cpu") {
            xTaskCreatePinnedToCore(getCoreFreq, "get_freq", 2048, nullptr,
                                    1, nullptr, 0);
            Serial.printf("Core %d Frequency %d MHz\n", xPortGetCoreID(), ets_get_cpu_frequency());
        }
    }
}

void initFmtVars() {
    Serial.printf("[Pager] Processing time %llu ms.\n", millis64() - runtime_timer);
    runtime_timer = 0;
    rxInfo.rssi = 0;
    rxInfo.fer = 0;
    rxInfo.ppm = 0;
    if (db != nullptr) {
        delete db;
        db = nullptr;
    }
}

void formatDataTask(void *pVoid) {
    fd_state = TASK_RUNNING;
    sd1.append(2, "格式化任务已创建\n");
    for (auto &i: db->pocsagData) {
        if (i.is_empty)
            continue;
        Serial.printf("[D-pDATA] %d/%d: %s\n", i.addr, i.func, i.str.c_str());
        sd1.append(2, "[D-pDATA] %d/%d: %s\n", i.addr, i.func, i.str.c_str());
        db->str = db->str + "  " + i.str;
    }

    sd1.append(2, "原始数据输出完成，用时[%llu]\n", millis64() - runtime_timer);
    Serial.printf("decode complete.[%llu]", millis64() - runtime_timer);
    readDataLBJ(db->pocsagData, &db->lbjData);
    sd1.append(2, "LBJ读取完成，用时[%llu]\n", millis64() - runtime_timer);
    Serial.printf("Read complete.[%llu]", millis64() - runtime_timer);

    printDataSerial(db->pocsagData, db->lbjData, rxInfo);
    sd1.append(2, "串口输出完成，用时[%llu]\n", millis64() - runtime_timer);
    Serial.printf("SPRINT complete.[%llu]", millis64() - runtime_timer);

    appendDataLog(db->pocsagData, db->lbjData, rxInfo);
    Serial.printf("sdprint complete.[%llu]", millis64() - runtime_timer);
    appendDataCSV(db->pocsagData, db->lbjData, rxInfo);
    Serial.printf("csvprint complete.[%llu]", millis64() - runtime_timer);

    printDataTelnet(db->pocsagData, db->lbjData, rxInfo);
    Serial.printf("telprint complete.[%llu]", millis64() - runtime_timer);

#ifdef HAS_DISPLAY
    fd_state = TASK_RUNNING_SCREEN;
    if (u8g2) {
#ifdef HAS_OLED_TIMEOUT
        if (oled_off) {
            oled_off = false;
            u8g2->setPowerSave(false);
            u8g2->clearBuffer();
            updateInfo(actual_frequency);
        }
#endif
        if (db->lbjData.type == 0)
            showLBJ0(db->lbjData, rxInfo);
        else if (db->lbjData.type == 1)
            showLBJ1(db->lbjData, rxInfo);
        else if (db->lbjData.type == 2)
            showLBJ2(db->lbjData, rxInfo);
        Serial.printf("Complete u8g2 [%llu]\n", millis64() - runtime_timer);
    }
#endif
    Serial.printf("[FD-Task] Stack High Mark %u\n", uxTaskGetStackHighWaterMark(nullptr));
    sd1.append(2, "任务堆栈标 %u\n", uxTaskGetStackHighWaterMark(nullptr));
    sd1.append(2, "格式化输出任务完成，用时[%llu]\n", millis64() - runtime_timer);
    fd_state = TASK_DONE;
    task_fd = nullptr;
    vTaskDelete(nullptr);
}

void simpleFormatTask() {
    for (auto &i: db->pocsagData) {
        if (i.is_empty)
            continue;
        Serial.printf("[D-pDATA] %d/%d: %s\n", i.addr, i.func, i.str.c_str());
        sd1.append("[D-pDATA] %d/%d: %s\n", i.addr, i.func, i.str.c_str());
        db->str += String(i.addr) + "/" + String(i.func) + ":" + i.str + "\n ";
    }
#ifdef HAS_OLED_TIMEOUT
    if (oled_off) {
        oled_off = false;
        u8g2->setPowerSave(false);
        u8g2->clearBuffer();
        updateInfo(actual_frequency);
    }
#endif
    showSTR(db->str);
}
