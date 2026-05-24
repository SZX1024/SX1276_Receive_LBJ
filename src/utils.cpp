#include "utils.h"

String printResetReason(esp_reset_reason_t reset) {
    String str;
    switch (reset) {
        case ESP_RST_UNKNOWN:
            str = "ESP_RST_UNKNOWN, Reset reason can not be determined";
            break;
        case ESP_RST_POWERON:
            str = "ESP_RST_POWERON, Reset due to power-on event";
            break;
        case ESP_RST_EXT:
            str = "ESP_RST_EXT, Reset by external pin (not applicable for ESP32)";
            break;
        case ESP_RST_SW:
            str = "ESP_RST_SW, Software reset via esp_restart";
            break;
        case ESP_RST_PANIC:
            str = "ESP_RST_PANIC, Software reset due to exception/panic";
            break;
        case ESP_RST_INT_WDT:
            str = "ESP_RST_INT_WDT, Reset (software or hardware) due to interrupt watchdog";
            break;
        case ESP_RST_TASK_WDT:
            str = "ESP_RST_TASK_WDT, Reset due to task watchdog";
            break;
        case ESP_RST_WDT:
            str = "ESP_RST_WDT, Reset due to other watchdogs";
            break;
        case ESP_RST_DEEPSLEEP:
            str = "ESP_RST_DEEPSLEEP, Reset after exiting deep sleep mode";
            break;
        case ESP_RST_BROWNOUT:
            str = "ESP_RST_BROWNOUT, Brownout reset (software or hardware)";
            break;
        case ESP_RST_SDIO:
            str = "ESP_RST_SDIO, Reset over SDIO";
            break;
    }
    return str;
}

char *fmtime(const struct tm &time) {
    static char buffer[20];
    sprintf(buffer, "%d-%02d-%02d %02d:%02d:%02d", time.tm_year + 1900, time.tm_mon + 1, time.tm_mday, time.tm_hour,
            time.tm_min, time.tm_sec);
    return buffer;
}

char *fmtms(uint64_t ms) {
    static char buffer[40];
    if (ms < 60000)
        sprintf(buffer, "%.3f Seconds", (double) ms / 1000);
    else if (ms < 3600000)
        sprintf(buffer, "%llu Minutes %.3f Seconds", ms / 60000, (double) (ms % 60000) / 1000);
    else if (ms < 86400000)
        sprintf(buffer, "%llu Hours %llu Minutes %.3f Seconds", ms / 3600000, ms % 3600000 / 60000,
                (double) (ms % 3600000 % 60000) / 1000);
    else
        sprintf(buffer, "%llu Days %llu Hours %llu Minutes %.3f Seconds", ms / 86400000, ms % 86400000 / 3600000,
                ms % 86400000 % 3600000 / 60000, (double) (ms % 86400000 % 3600000 % 60000) / 1000);

    return buffer;
}
