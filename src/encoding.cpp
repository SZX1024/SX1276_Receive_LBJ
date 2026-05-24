#include "encoding.h"
#include "unicon.hpp"
#include <assert.h>

int enc_unicode_to_utf8_one(unsigned long unic, unsigned char *pOutput) {
    assert(pOutput != nullptr);

    if (unic <= 0x0000007F) {
        *pOutput = (unic & 0x7F);
        return 1;
    } else if (unic >= 0x00000080 && unic <= 0x000007FF) {
        *(pOutput + 1) = (unic & 0x3F) | 0x80;
        *pOutput = ((unic >> 6) & 0x1F) | 0xC0;
        return 2;
    } else if (unic >= 0x00000800 && unic <= 0x0000FFFF) {
        *(pOutput + 2) = (unic & 0x3F) | 0x80;
        *(pOutput + 1) = ((unic >> 6) & 0x3F) | 0x80;
        *pOutput = ((unic >> 12) & 0x0F) | 0xE0;
        return 3;
    } else if (unic >= 0x00010000 && unic <= 0x001FFFFF) {
        *(pOutput + 3) = (unic & 0x3F) | 0x80;
        *(pOutput + 2) = ((unic >> 6) & 0x3F) | 0x80;
        *(pOutput + 1) = ((unic >> 12) & 0x3F) | 0x80;
        *pOutput = ((unic >> 18) & 0x07) | 0xF0;
        return 4;
    } else if (unic >= 0x00200000 && unic <= 0x03FFFFFF) {
        *(pOutput + 4) = (unic & 0x3F) | 0x80;
        *(pOutput + 3) = ((unic >> 6) & 0x3F) | 0x80;
        *(pOutput + 2) = ((unic >> 12) & 0x3F) | 0x80;
        *(pOutput + 1) = ((unic >> 18) & 0x3F) | 0x80;
        *pOutput = ((unic >> 24) & 0x03) | 0xF8;
        return 5;
    } else if (unic >= 0x04000000 && unic <= 0x7FFFFFFF) {
        *(pOutput + 5) = (unic & 0x3F) | 0x80;
        *(pOutput + 4) = ((unic >> 6) & 0x3F) | 0x80;
        *(pOutput + 3) = ((unic >> 12) & 0x3F) | 0x80;
        *(pOutput + 2) = ((unic >> 18) & 0x3F) | 0x80;
        *(pOutput + 1) = ((unic >> 24) & 0x3F) | 0x80;
        *pOutput = ((unic >> 30) & 0x01) | 0xFC;
        return 6;
    }

    return 0;
}

#define MAX_GBK_LEN 64

void gbk2utf8(const char *gbk1, char *utf8s, size_t gbk_len) {
    if (gbk_len > MAX_GBK_LEN) gbk_len = MAX_GBK_LEN;
    uint16_t unic[MAX_GBK_LEN];
    uint8_t gbk[MAX_GBK_LEN];

    size_t c = 0;
    for (size_t i = 0; i < gbk_len; i++) {
        gbk[i] = (uint8_t) gbk1[i];
    }
    for (size_t i = 0; i < gbk_len; i++, c++) {
        if (gbk[i] < 0x80) {
            unic[c] = gbk[i];
        } else {
            unic[c] = ff_oem2uni((uint16_t) (gbk[i] << 8 | gbk[i + 1]), 936);
            i++;
        }
    }
    c = 0;
    size_t i = 0;
    uint8_t utf8[MAX_GBK_LEN * 2];
    size_t utf8_max = MAX_GBK_LEN * 2;
    for (; i < utf8_max; i++, c++) {
        uint8_t ut8[4];
        int r = enc_unicode_to_utf8_one(unic[c], ut8);
        if (i + 4 < utf8_max) {
            if (r == 1) utf8[i] = ut8[0];
            else if (r == 2) { utf8[i] = ut8[0]; utf8[++i] = ut8[1]; }
            else if (r == 3) { utf8[i] = ut8[0]; utf8[++i] = ut8[1]; utf8[++i] = ut8[2]; }
            else if (r == 4) { utf8[i] = ut8[0]; utf8[++i] = ut8[1]; utf8[++i] = ut8[2]; utf8[++i] = ut8[3]; }
        }
    }
    size_t out_len = gbk_len * 2;
    for (size_t v = 0; v < out_len; v++) {
        utf8s[v] = (char) utf8[v];
    }
}

void gbk2utf8(const uint8_t *gbk, uint8_t *utf8, size_t gbk_len) {
    gbk2utf8((const char *) gbk, (char *) utf8, gbk_len);
}
