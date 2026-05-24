#include "output.h"
#include "radio.h"
#include "netmgr.h"
#include "sdlog.hpp"

void printDataSerial(PagerClient::pocsag_data *p, const struct lbj_data &l, const struct rx_info &r) {
    switch (l.type) {
        case 0: {
            if (l.direction == FUNCTION_UP)
                Serial.print("[LBJ] 方向: 上行  ");
            else if (l.direction == FUNCTION_DOWN)
                Serial.print("[LBJ] 方向: 下行  ");
            else
                Serial.printf("[LBJ] 方向: %4d  ", l.direction);
            Serial.printf("车次: %s  速度: %s KM/H  公里标: %s KM  ", l.train, l.speed, l.position);
            break;
        }
        case 1: {
            Serial.printf("==================================================================================\n");
            if (l.direction == FUNCTION_UP)
                Serial.print("[LBJ] 方向: 上行     ");
            else if (l.direction == FUNCTION_DOWN)
                Serial.print("[LBJ] 方向: 下行     ");
            else
                Serial.printf("[LBJ] 方向: %3d     ", l.direction);
            Serial.printf("车次: %s%s   速度: %s KM/H  公里标: %s KM \n", l.lbj_class, l.train, l.speed, l.position);
            Serial.printf("[LBJ] 线路: %s 车号: %s  ", l.route_utf8, l.loco);
            if (l.pos_lat_deg[1] && l.pos_lat_min[1])
                Serial.printf("位置: %s°%2s′ ", l.pos_lat_deg, l.pos_lat_min);
            else
                Serial.printf("位置: %s ", l.pos_lat);
            if (l.pos_lon_deg[1] && l.pos_lon_min[1])
                Serial.printf("%s°%2s′ \n", l.pos_lon_deg, l.pos_lon_min);
            else
                Serial.printf("%s \n", l.pos_lon);
            Serial.printf("----------------------------------------------------------------------------------\n");
            Serial.printf("[RXI] [R:%3.1f dBm/F:%4.2f Hz/%.2f ppm/P:%.2f]\n", r.rssi, r.fer,
                          getBias((float) (actual_frequency + r.fer * 1e-6)), r.ppm);
            for (size_t i = 0; i < POCDAT_SIZE; i++) {
                if (p[i].is_empty)
                    continue;
                Serial.printf("[PGR] [%d/%d:%s]", p[i].addr, p[i].func, p[i].str.c_str());
                Serial.printf("[E:%02d/%02d/%zu]\n", p[i].errs_uncorrected, p[i].errs_total, (p[i].len / 5) * 32);
            }
            Serial.printf("==================================================================================\n");
            break;
        }
        case 2: {
            Serial.printf("[LBJ] 当前时间 %s  ", l.time);
            break;
        }
    }
    if (l.type != 1) {
        Serial.printf("[R:%3.1f dBm/F:%5.2f Hz/%.2f ppm/P:%.2f]\n", r.rssi, r.fer,
                      getBias((float) (actual_frequency + r.fer * 1e-6)), r.ppm);
        for (size_t i = 0; i < POCDAT_SIZE; i++) {
            if (p[i].is_empty)
                continue;
            Serial.printf("[%d/%d:%s]", p[i].addr, p[i].func, p[i].str.c_str());
            Serial.printf("[E:%02d/%02d/%zu]", p[i].errs_uncorrected, p[i].errs_total, (p[i].len / 5) * 32);
        }
        Serial.printf("\n");
    }
}

void appendDataLog(PagerClient::pocsag_data *p, const struct lbj_data &l, const struct rx_info &r) {
    switch (l.type) {
        case 0: {
            if (l.direction == FUNCTION_UP)
                sd1.appendBuffer("[LBJ] 方向: 上行  ");
            else if (l.direction == FUNCTION_DOWN)
                sd1.appendBuffer("[LBJ] 方向: 下行  ");
            else
                sd1.appendBuffer("[LBJ] 方向: %4d  ", l.direction);
            sd1.appendBuffer("车次: %s  速度: %s KM/H  公里标: %s KM  ", l.train, l.speed, l.position);
            break;
        }
        case 1: {
            sd1.appendBuffer(
                    "==================================================================================\n");
            if (l.direction == FUNCTION_UP)
                sd1.appendBuffer("[LBJ] 方向: 上行     ");
            else if (l.direction == FUNCTION_DOWN)
                sd1.appendBuffer("[LBJ] 方向: 下行     ");
            else
                sd1.appendBuffer("[LBJ] 方向: %3d     ", l.direction);
            sd1.appendBuffer("车次: %s%s   速度: %s KM/H  公里标: %s KM \n", l.lbj_class, l.train, l.speed,
                             l.position);
            sd1.appendBuffer("[LBJ] 线路: %s 车号: %s  ", l.route_utf8, l.loco);
            if (l.pos_lat_deg[1] && l.pos_lat_min[1])
                sd1.appendBuffer("位置: %s°%2s′ ", l.pos_lat_deg, l.pos_lat_min);
            else
                sd1.appendBuffer("位置: %s ", l.pos_lat);
            if (l.pos_lon_deg[1] && l.pos_lon_min[1])
                sd1.appendBuffer("%s°%2s′ \n", l.pos_lon_deg, l.pos_lon_min);
            else
                sd1.appendBuffer("%s \n", l.pos_lon);
            sd1.appendBuffer(
                    "----------------------------------------------------------------------------------\n");
            sd1.appendBuffer("[RXI] [R:%3.1f dBm/F:%4.2f Hz/%.2f ppm/P:%.2f]\n", r.rssi, r.fer,
                             getBias((float) (actual_frequency + r.fer * 1e-6)), r.ppm);
            for (size_t i = 0; i < POCDAT_SIZE; i++) {
                if (p[i].is_empty)
                    continue;
                sd1.appendBuffer("[PGR] [%d/%d:%s]", p[i].addr, p[i].func, p[i].str.c_str());
                sd1.appendBuffer("[E:%02d/%02d/%zu]\n", p[i].errs_uncorrected, p[i].errs_total,
                                 (p[i].len / 5) * 32);
            }
            sd1.appendBuffer(
                    "==================================================================================\n");
            break;
        }
        case 2: {
            sd1.appendBuffer("[LBJ] 当前时间 %s  ", l.time);
            break;
        }
    }
    if (l.type != 1) {
        sd1.appendBuffer("[R:%3.1f dBm/F:%5.2f Hz/%.2f ppm/P:%.2f]\n", r.rssi, r.fer,
                         getBias((float) (actual_frequency + r.fer * 1e-6)), r.ppm);
        for (size_t i = 0; i < POCDAT_SIZE; i++) {
            if (p[i].is_empty)
                continue;
            sd1.appendBuffer("[%d/%d:%s]", p[i].addr, p[i].func, p[i].str.c_str());
            sd1.appendBuffer("[E:%02d/%02d/%zu]", p[i].errs_uncorrected, p[i].errs_total, (p[i].len / 5) * 32);
        }
        sd1.appendBuffer("\n");
    }
    sd1.sendBufferLOG();
}

void printDataTelnet(PagerClient::pocsag_data *p, const struct lbj_data &l, const struct rx_info &r) {
    switch (l.type) {
        case 0: {
            if (l.direction == FUNCTION_UP)
                telPrintf(true, "[LBJ] 方向: 上行  ");
            else if (l.direction == FUNCTION_DOWN)
                telPrintf(true, "[LBJ] 方向: 下行  ");
            else
                telPrintf(true, "[LBJ] 方向: %3d  ", l.direction);
            telPrintf(true, "车次: %s  速度: %s KM/H  公里标: %s KM  ", l.train, l.speed, l.position);
            break;
        }
        case 1: {
            telPrintf(true, "==================================================================================\n");
            if (l.direction == FUNCTION_UP)
                telPrintf(true, "[LBJ] 方向: 上行     ");
            else if (l.direction == FUNCTION_DOWN)
                telPrintf(true, "[LBJ] 方向: 下行     ");
            else
                telPrintf(true, "[LBJ] 方向: %4d     ", l.direction);
            telPrintf(true, "车次: %s%s   速度: %s KM/H  公里标: %s KM \n", l.lbj_class, l.train, l.speed, l.position);
            telPrintf(true, "[LBJ] 线路: %s 车号: %s  ", l.route_utf8, l.loco);
            if (l.pos_lat_deg[1] && l.pos_lat_min[1])
                telPrintf(true, "位置: %s°%2s′ ", l.pos_lat_deg, l.pos_lat_min);
            else
                telPrintf(true, "位置: %s ", l.pos_lat);
            if (l.pos_lon_deg[1] && l.pos_lon_min[1])
                telPrintf(true, "%s°%2s′ \n", l.pos_lon_deg, l.pos_lon_min);
            else
                telPrintf(true, "%s \n", l.pos_lon);
            telPrintf(true, "----------------------------------------------------------------------------------\n");
            telPrintf(true, "[RXI] [R:%3.1f dBm/F:%4.2f Hz/%.2f ppm/P:%.2f]\n", r.rssi, r.fer,
                      getBias((float) (actual_frequency + r.fer * 1e-6)), r.ppm);
            for (size_t i = 0; i < POCDAT_SIZE; i++) {
                if (p[i].is_empty)
                    continue;
                telPrintf(true, "[PGR] [%d/%d:%s]", p[i].addr, p[i].func, p[i].str.c_str());
                telPrintf(true, "[E:%02d/%02d/%zu]\n", p[i].errs_uncorrected, p[i].errs_total, (p[i].len / 5) * 32);
            }
            telPrintf(true, "==================================================================================\n");
            break;
        }
        case 2: {
            telPrintf(true, "[LBJ] 当前时间 %s  ", l.time);
            break;
        }
    }
    if (l.type != 1) {
        telPrintf(true, "[R:%3.1f dBm/F:%5.2f Hz/%.2f ppm/P:%.2f]\n", r.rssi, r.fer,
                  getBias((float) (actual_frequency + r.fer * 1e-6)), r.ppm);
    }
}

void appendDataCSV(PagerClient::pocsag_data *p, const struct lbj_data &l, const struct rx_info &r) {
    switch (l.type) {
        case 0: {
            if (l.direction == FUNCTION_UP)
                sd1.appendBufferCSV(",上行,");
            else if (l.direction == FUNCTION_DOWN)
                sd1.appendBufferCSV(",下行,");
            else
                sd1.appendBufferCSV(",%d,", l.direction);
            sd1.appendBufferCSV(",%s,%s,%s,,,,,,", l.train, l.speed, l.position);
            break;
        }
        case 1: {
            if (l.direction == FUNCTION_UP)
                sd1.appendBufferCSV(",上行,");
            else if (l.direction == FUNCTION_DOWN)
                sd1.appendBufferCSV(",下行,");
            else
                sd1.appendBufferCSV(",%d,", l.direction);
            sd1.appendBufferCSV("%s,%s,%s,%s,%s,%s,", l.lbj_class, l.train, l.speed, l.position, l.loco,
                                l.route_utf8);
            if (l.pos_lat_deg[1] && l.pos_lat_min[1])
                sd1.appendBufferCSV("%s°%2s′,", l.pos_lat_deg, l.pos_lat_min);
            else
                sd1.appendBufferCSV("%s,", l.pos_lat);
            if (l.pos_lon_deg[1] && l.pos_lon_min[1])
                sd1.appendBufferCSV("%s°%2s′,", l.pos_lon_deg, l.pos_lon_min);
            else
                sd1.appendBufferCSV("%s,", l.pos_lon);
            sd1.appendBufferCSV("\"%s\",%3.1f,%4.2f,%.2f,%.2f,", l.info2_hex.c_str(), r.rssi, r.fer,
                                getBias((float) (actual_frequency + r.fer * 1e-6)), r.ppm);
            sd1.appendBufferCSV("\"");
            uint8_t err_ttl = 0, err_un = 0, len = 0;
            for (size_t i = 0; i < POCDAT_SIZE; i++) {
                if (p[i].is_empty)
                    continue;
                sd1.appendBufferCSV("[%d/%d:%s][E:%02d/%02d/%zu]", p[i].addr, p[i].func, p[i].str.c_str(),
                                    p[i].errs_uncorrected, p[i].errs_total, (p[i].len / 5) * 32);
                err_ttl += p[i].errs_total;
                err_un += p[i].errs_uncorrected;
                len += (p[i].len / 5) * 32;
            }
            sd1.appendBufferCSV("\",");
            sd1.appendBufferCSV("%d/%d,%.2f%%\n", err_un, err_ttl, ((float) err_ttl / (float) len) * 100);
            break;
        }
        case 2: {
            if (strcmp(l.time, "<NUL>") != 0)
                sd1.appendBufferCSV("%s,,,,,,,,,,,", l.time);
            else
                sd1.appendBufferCSV("null,,,,,,,,,,,");
            break;
        }
    }
    if (l.type != 1 && !p[0].is_empty) {
        sd1.appendBufferCSV("%3.1f,%5.2f,%.2f,%.2f,\"", r.rssi, r.fer,
                            getBias((float) (actual_frequency + r.fer * 1e-6)), r.ppm);
        uint8_t err_un = 0, err_ttl = 0, len = 0;
        for (size_t i = 0; i < POCDAT_SIZE; i++) {
            if (p[i].is_empty)
                continue;
            sd1.appendBufferCSV("[%d/%d:%s][E:%02d/%02d/%zu]", p[i].addr, p[i].func, p[i].str.c_str(),
                                p[i].errs_uncorrected, p[i].errs_total, (p[i].len / 5) * 32);
            err_un += p[i].errs_uncorrected;
            err_ttl += p[i].errs_total;
            len += (p[i].len / 5) * 32;
        }
        sd1.appendBufferCSV("\",");
        sd1.appendBufferCSV("%d/%d,%.2f%%\n", err_un, err_ttl, ((float) err_ttl / (float) len) * 100);
    }
    sd1.sendBufferCSV();
}
