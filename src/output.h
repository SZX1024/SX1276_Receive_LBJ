#pragma once

#include "config.h"

void printDataSerial(PagerClient::pocsag_data *p, const struct lbj_data &l, const struct rx_info &r);
void appendDataLog(PagerClient::pocsag_data *p, const struct lbj_data &l, const struct rx_info &r);
void printDataTelnet(PagerClient::pocsag_data *p, const struct lbj_data &l, const struct rx_info &r);
void appendDataCSV(PagerClient::pocsag_data *p, const struct lbj_data &l, const struct rx_info &r);
