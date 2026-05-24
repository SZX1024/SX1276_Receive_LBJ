#include "protocol.h"
#include "encoding.h"
#include "loco.h"
#include <string>

int8_t hexToChar(int8_t hex1, int8_t hex2) {
    uint8_t a, b;
    if (isdigit(hex1))
        a = hex1 - 0x30;
    else
        a = hex1 - 0x37;
    if (isdigit(hex2))
        b = hex2 - 0x30;
    else
        b = hex2 - 0x37;

    return (int8_t) ((a << 4) | b);
}

void recodeBCD(const char *c, String *v) {
    switch (*c) {
        case '*': {
            *v += 'A';
            break;
        }
        case 'U': {
            *v += 'B';
            break;
        }
        case ' ': {
            *v += 'C';
            break;
        }
        case '-': {
            *v += 'D';
            break;
        }
        case ')': {
            *v += 'E';
            break;
        }
        case '(': {
            *v += 'F';
            break;
        }
        default: {
            *v += *c;
            break;
        }
    }
}

int16_t readDataLBJ(struct PagerClient::pocsag_data *p, struct lbj_data *l) {
    for (size_t i = 0; i < POCDAT_SIZE; i++) {
        if (p[i].is_empty)
            continue;
        switch (p[i].addr) {
            case LBJ_INFO_ADDR: {
                if (!p[i].func && p[i].len == 5 && (p[i].str[0] == '-' || p[i].str[0] == '*') &&
                    (p[i].str[1] && p[i].str[2] && p[i].str[3] && p[i].str[4] != '-')) {

                    p[i].addr = LBJ_SYNC_ADDR;
                    Serial.println("Transformed type.");
                    goto lbj_sync;
                } else {
                    l->type = 0;
                    l->direction = (int8_t) p[i].func;

                    if (p[i].str.length() >= 5 && p[i].str[0] != 'X') {
                        for (size_t c = 0; c < 5; c++) {
                            l->train[c] = p[i].str[c];
                        }
                    }
                    if (p[i].str.length() >= 10 && p[i].str[6] != 'X') {
                        for (size_t c = 6, v = 0; c < 9; c++, v++) {
                            l->speed[v] = p[i].str[c];
                        }
                        l->speed[3] = 0;
                    }
                    if (p[i].str.length() >= 15 && p[i].str[10] != 'X') {
                        size_t v = 0;
                        for (size_t c = 10; c < 15; c++, v++) {
                            l->position[v] = p[i].str[c];
                            if (c == 13 && p[i].str[c] != ' ') {
                                l->position[++v] = '.';
                            } else if (c == 13) {
                                l->position[v] = '0';
                                l->position[++v] = '.';
                            }
                        }
                        l->position[v] = 0;
                    }
                    if (!(p[i].str.length() == 65 || p[i].str.length() >= 70)) {
                        break;
                    }
                }
            }
            case LBJ_INFO2_ADDR: {
                l->type = 1;
                String buffer;
                if (p[i].addr == LBJ_INFO_ADDR) {
                    if (p[i].str.length() == 65)
                        buffer = p[i].str.substring(15);
                    else if (p[i].str.length() == 70 || (p[i].str.length() >= 70 && p[i].str[67] == '0'
                                                         && p[i].str[68] == '0' && p[i].str[69] == '0'))
                        buffer = p[i].str.substring(20);
                    else
                        break;
                } else
                    buffer = p[i].str;
                if (l->direction == -1)
                    l->direction = (int8_t) p[i].func;

                for (char &c: buffer) {
                    recodeBCD(&c, &l->info2_hex);
                }

                if (buffer.length() >= 12 && buffer[4] != 'X' && buffer[5] != 'X' && buffer[10] != 'X') {
                    for (size_t c = 4, v = 0; c < 12; c++, v++) {
                        l->loco[v] = buffer[c];
                    }
                    String type;
                    for (size_t c = 0; c < 3; c++) {
                        if (isdigit(l->loco[c]))
                            type += l->loco[c];
                    }
                    if (type.length() == 3 && type.toInt() < (sizeof locos / sizeof locos[0]) && type.toInt() >= 0) {
                        l->loco_type = locos[std::stoi(type.c_str())];
                    }
                }

                if (buffer.length() >= 39 && buffer[30] != 'X' && buffer[35] != 'X') {
                    for (size_t c = 30, v = 0; c < 39; c++, v++) {
                        l->pos_lon[v] = buffer[c];
                    }
                    for (size_t c = 30, v = 0; c < 33; c++, v++) {
                        l->pos_lon_deg[v] = buffer[c];
                    }
                    size_t v = 0;
                    for (size_t c = 33; c < 39; c++, v++) {
                        l->pos_lon_min[v] = buffer[c];
                        if (c == 34)
                            l->pos_lon_min[++v] = '.';
                    }
                }
                if (buffer.length() >= 47 && buffer[39] != 'X' && buffer[40] != 'X' && buffer[45] != 'X') {
                    for (size_t c = 39, v = 0; c < 47; c++, v++) {
                        l->pos_lat[v] = buffer[c];
                    }
                    for (size_t c = 39, v = 0; c < 41; c++, v++) {
                        l->pos_lat_deg[v] = buffer[c];
                    }
                    size_t v = 0;
                    for (size_t c = 41; c < 47; c++, v++) {
                        l->pos_lat_min[v] = buffer[c];
                        if (c == 42)
                            l->pos_lat_min[++v] = '.';
                    }
                }

                if (l->info2_hex.length() >= 4 && l->info2_hex[0] != 'X') {
                    size_t c = 0;
                    for (size_t v = 0; v < 3; v++, c++) {
                        int8_t ch = hexToChar(l->info2_hex[v], l->info2_hex[v + 1]);
                        ++v;
                        if (ch > 0x1F && ch < 0x7F && ch != 0x22 && ch != 0x2C)
                            l->lbj_class[c] = ch;
                    }
                }
                if (l->info2_hex.length() >= 20 && l->info2_hex[14] != 'X' && l->info2_hex[15] != 'X') {
                    size_t c = 0;
                    for (size_t v = 14; v < 17; v++, c++) {
                        int8_t ch = hexToChar(l->info2_hex[v], l->info2_hex[v + 1]);
                        ++v;
                        if ((uint8_t )ch >= 0xA0 || ch == 0x20 || ch >= 0x2D && ch <= 0x7E && ch != 0x60)
                            l->route[c] = ch;
                    }
                }
                if (l->info2_hex.length() >= 25 && l->info2_hex[18] != 'X' && l->info2_hex[20] != 'X') {
                    size_t c = 2;
                    for (size_t v = 18; v < 21; v++, c++) {
                        int8_t ch = hexToChar(l->info2_hex[v], l->info2_hex[v + 1]);
                        ++v;
                        if ((uint8_t )ch >= 0xA0 || ch == 0x20 || ch >= 0x2D && ch <= 0x7E && ch != 0x60)
                            l->route[c] = ch;
                    }
                }
                if (l->info2_hex.length() >= 30 && l->info2_hex[22] != 'X' && l->info2_hex[25] != 'X') {
                    size_t c = 4;
                    for (size_t v = 22; v < 29; v++, c++) {
                        int8_t ch = hexToChar(l->info2_hex[v], l->info2_hex[v + 1]);
                        ++v;
                        if ((uint8_t )ch >= 0xA0 || ch == 0x20 || ch >= 0x2D && ch <= 0x7E && ch != 0x60)
                            l->route[c] = ch;
                    }
                }
                gbk2utf8(l->route, l->route_utf8, 17);
                break;
            }
            case LBJ_SYNC_ADDR: {
                lbj_sync:
                l->type = 2;
                if (p[i].str.length() >= 5 && p[i].str[0] != 'X') {
                    for (size_t c = 1, v = 0; c < 5; c++, v++) {
                        l->time[v] = p[i].str[c];
                        if (c == 2)
                            l->time[++v] = ':';
                    }
                }
                break;
            }
        }
    }

    return 0;
}
