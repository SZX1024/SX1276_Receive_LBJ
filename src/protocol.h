#pragma once

#include "config.h"

int16_t readDataLBJ(struct PagerClient::pocsag_data *p, struct lbj_data *l);

int8_t hexToChar(int8_t hex1, int8_t hex2);

void recodeBCD(const char *c, String *v);
