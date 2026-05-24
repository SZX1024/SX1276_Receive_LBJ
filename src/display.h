#pragma once

#include "config.h"

extern bool oled_off;
extern uint64_t screen_timer;
extern uint64_t timer4;
extern bool low_volt_warned;

#ifdef HAS_DISPLAY
void pword(const char *msg, int xloc, int yloc);
void showInitComp();
void updateInfo(float freq);
void showSTR(const String &str);
void showLBJ0(const struct lbj_data &l, const struct rx_info &r);
void showLBJ1(const struct lbj_data &l, const struct rx_info &r);
void showLBJ2(const struct lbj_data &l, const struct rx_info &r);
#endif
