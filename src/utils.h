#pragma once

#include <Arduino.h>
#include <ctime>

char *fmtime(const struct tm &time);

char *fmtms(uint64_t ms);

String printResetReason(esp_reset_reason_t reset);
