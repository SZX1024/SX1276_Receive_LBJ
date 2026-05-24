#pragma once

#include <cstdint>
#include <cstddef>

int enc_unicode_to_utf8_one(unsigned long unic, unsigned char *pOutput);

void gbk2utf8(const uint8_t *gbk, uint8_t *utf8, size_t gbk_len);

void gbk2utf8(const char *gbk1, char *utf8s, size_t gbk_len);
