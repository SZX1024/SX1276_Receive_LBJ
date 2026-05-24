#include "radio.h"
#include "BCH3121.hpp"
#include "sdlog.hpp"
#include "boards.hpp"

// SX1276 instances
SX1276 radio = new Module(RADIO_CS_PIN, RADIO_DIO0_PIN, RADIO_RST_PIN, RADIO_DIO1_PIN);
PagerClient pager(&radio);
const int pin = RADIO_BUSY_PIN;

// AFC / radio state
float rssi_cache = 0;
float fers[32]{};
float actual_frequency = 0;
float freq_last = 0;
float car_fer_last = 0;
float ppm = INITIAL_PPM;
bool freq_correction = AFC_ENABLE;
uint64_t prb_timer = 0;
uint32_t prb_count = 0;
uint64_t car_timer = 0;
uint32_t car_count = 0;
struct rx_info rxInfo;

// Telnet async relay flags
bool give_tel_rssi = false;
bool give_tel_gain = false;
bool tel_set_ppm = false;

/* ------------------------------------------------ */
/* Radio init and AFC                                */
/* ------------------------------------------------ */

float getBias(float freq) {
    return (float) ((freq - TARGET_FREQ) * 1e6 / TARGET_FREQ);
}

int initPager() {
    int state = radio.beginFSK(434.0, 4.8, 5.0, 12.5);
    RADIOLIB_ASSERT(state)

    state = radio.setGain(1);
    RADIOLIB_ASSERT(state)

    state = pager.begin(actualFreq(ppm), 1200, false, 2500);
    RADIOLIB_ASSERT(state)

    freq_last = actual_frequency;

    state = pager.startReceive(pin, 1234000, 0xFFFF0);
    RADIOLIB_ASSERT(state)

    return (state);
}

void revertFrequency() {
    if (actual_frequency != freq_last) {
        actual_frequency = freq_last;
        int state = radio.setFrequency(actual_frequency);
        if (state != RADIOLIB_ERR_NONE) {
            Serial.printf("[D] Revert freq failed %d\n", state);
        } else {
            Serial.printf("[D] Revert to last freq %f MHz, ppm %.2f\n", actual_frequency, getBias(actual_frequency));
        }
    }
}

static void afcTryAdjust(float fei, float fei_last, uint32_t count, const char *tag) {
    if (abs(fei) > 1000.0 && count != 1 && abs(fei - fei_last) < 500) {
        auto target_freq = (float) (actual_frequency + fei * 1e-6);
        int state = radio.setFrequency(target_freq);
        if (state != RADIOLIB_ERR_NONE) {
            Serial.printf("[D][%s] Freq Alter failed %d, target freq %f\n", tag, state, target_freq);
            sd1.append("[D][%s] Freq Alter failed %d, target freq %f\n", tag, state, target_freq);
        } else {
            actual_frequency = target_freq;
            Serial.printf("[D][%s] Freq Altered %f MHz, FEI %.2f Hz, PPM %.2f\n", tag, actual_frequency, fei,
                          getBias(actual_frequency));
        }
    }
}

void handleCarrier() {
    if (pager.gotCarrierState() && !pager.gotPreambleState() && !pager.gotSyncState() && freq_correction &&
        prb_timer == 0) {
        if (car_count == 0)
            car_timer = millis64();
        ++car_count;
        if (car_count < 64) {
            float fei = radio.getFrequencyError();
            afcTryAdjust(fei, car_fer_last, car_count, "C");
            car_fer_last = fei;
        }
    }
}

void handlePreamble() {
    if (pager.gotPreambleState() && !pager.gotSyncState() && freq_correction) {
        if (prb_count == 0)
            prb_timer = millis64();
        ++prb_count;
        if (prb_count < 32) {
            fers[prb_count - 1] = radio.getFrequencyError();
            float fei = fers[prb_count - 1];
            float last = (prb_count > 1) ? fers[prb_count - 2] : fei;
            afcTryAdjust(fei, last, prb_count, "P");
        }
    }
}

void handleSync() {
    if (pager.gotSyncState()) {
        if (rxInfo.cnt < 5 && (rxInfo.timer == 0 || esp_timer_get_time() - rxInfo.timer < 11000)) {
            float rssi = radio.getRSSI(false, true);
            rxInfo.timer = esp_timer_get_time();
            rssi_cache += rssi;
            rxInfo.cnt++;
            Serial.printf("[D] RXI %.2f\n", rssi_cache / (float) rxInfo.cnt);
        }
        if (rxInfo.fer == 0)
            rxInfo.fer = radio.getFrequencyError();
    }
}

/* ------------------------------------------------ */
/* PagerClient extensions (merged from PagerMod.cpp) */
int16_t PagerClient::readDataMSA(struct PagerClient::pocsag_data *p, size_t len) {
    int16_t state = RADIOLIB_ERR_NONE;
    bool complete = false;
    uint8_t framePos = 0;
    uint32_t addr_next = 0;
    for (size_t i = 0; i < POCDAT_SIZE; i++) {
        size_t length = len;
        if (len == 0) {
            len = available() * 80;
        }

        if (complete)
            break;

#if defined(RADIOLIB_STATIC_ONLY)
        uint8_t data[RADIOLIB_STATIC_ARRAY_SIZE + 1];
#else
#endif
        uint8_t data[len + 1];

        state = readDataMA(data, &length, &p[i].addr, &p[i].func, &framePos, &addr_next, &p[i].is_empty,
                           &complete, &p[i].errs_total, &p[i].errs_uncorrected);

        if (i && state == RADIOLIB_ERR_ADDRESS_NOT_FOUND) {
            state = RADIOLIB_ERR_NONE;
            break;
        }

        if (i && state == RADIOLIB_ERR_MSG_CORRUPT) {
            state = RADIOLIB_ERR_NONE;
            break;
        }

        if (state == RADIOLIB_ERR_NONE && !p[i].is_empty) {
            if (length == 0) {
                p[i].is_empty = true;
            }
            data[length] = 0;
            p[i].str = String((char *) data);
            if (!p[i].str.length())
                p[i].is_empty = true;
            p[i].len = length;
        }
        if (state != RADIOLIB_ERR_NONE)
            break;
    }

    return (state);
}

int16_t PagerClient::readDataMA(uint8_t *data, size_t *len, uint32_t *addr, uint32_t *func, uint8_t *framePos,
                                uint32_t *addr_next, bool *is_empty, bool *complete, uint16_t *errs_total,
                                uint16_t *errs_uncorrected) {
    bool match = false;
    uint8_t symbolLength = 0;
    uint16_t errors = 0;
    bool parity_check = true;
    uint16_t err_prev;
    uint32_t cw_prev;
    CBCH3121 PocsagFec;
    bool is_sync = true;

    if (*addr_next) {
        uint32_t addr_found =
                ((*addr_next & RADIOLIB_PAGER_ADDRESS_BITS_MASK) >> (RADIOLIB_PAGER_ADDRESS_POS - 3)) | (*framePos / 2);
        if ((addr_found & filterMask) == (filterAddr & filterMask)) {
            *is_empty = false;
            match = true;
            symbolLength = 4;
            *addr = addr_found;
            *func = (*addr_next & RADIOLIB_PAGER_FUNCTION_BITS_MASK) >> RADIOLIB_PAGER_FUNC_BITS_POS;
            *addr_next = 0;
        } else {
            return (RADIOLIB_ERR_ADDRESS_NOT_FOUND);
        }
    }

    while (!match && phyLayer->available()) {

        uint32_t cw = read();
        *framePos = *framePos + 1;

        err_prev = errors;
        cw_prev = cw;
        if (!PocsagFec.decode(cw, errors, parity_check)) {
            *errs_uncorrected += errors - err_prev;
            if (!is_sync) {
                *errs_total = errors;
                return (RADIOLIB_ERR_MSG_CORRUPT);
            }
            is_sync = false;
            continue;
        } else {
            if (!parity_check) {
                *errs_uncorrected += errors - err_prev;
                if (!is_sync) {
                    *errs_total = errors;
                    return (RADIOLIB_ERR_MSG_CORRUPT);
                }
                is_sync = false;
                parity_check = true;
                continue;
            }
            is_sync = true;
        }

        if (cw == RADIOLIB_PAGER_IDLE_CODE_WORD) {
            continue;
        }

        if (cw == RADIOLIB_PAGER_FRAME_SYNC_CODE_WORD) {
            *framePos = 0;
            continue;
        }

        if (cw & (RADIOLIB_PAGER_MESSAGE_CODE_WORD << (RADIOLIB_PAGER_CODE_WORD_LEN - 1))) {
            continue;
        }

        uint32_t addr_found =
                ((cw & RADIOLIB_PAGER_ADDRESS_BITS_MASK) >> (RADIOLIB_PAGER_ADDRESS_POS - 3)) | (*framePos / 2);
        if ((addr_found & filterMask) == (filterAddr & filterMask)) {
            *is_empty = false;
            match = true;
            if (addr) {
                *addr = addr_found;
                *func = (cw & RADIOLIB_PAGER_FUNCTION_BITS_MASK) >> RADIOLIB_PAGER_FUNC_BITS_POS;
            }
            symbolLength = 4;
        }
    }

    if (!match) {
        return (RADIOLIB_ERR_ADDRESS_NOT_FOUND);
    }

    size_t decodedBytes = 0;
    size_t deco = 0;
    uint32_t prevCw = 0;
    bool overflow = false;
    int8_t ovfBits = 0;
    errors = 0;
    while (!*complete && phyLayer->available()) {
        *framePos = *framePos + 1;
        uint32_t cw = read();

        err_prev = errors;
        cw_prev = cw;
        if (PocsagFec.decode(cw, errors, parity_check)) {
            if (!parity_check) {
                for (size_t i = 0; i < 5; i++) {
                    data[decodedBytes++] = 'X';
                }
                *errs_uncorrected += errors - err_prev;
                if (!is_sync) {
                    *errs_total = errors;
                    if (deco != 0)
                        goto end;
                    return (RADIOLIB_ERR_MSG_CORRUPT);
                }
                is_sync = false;
                parity_check = true;
                continue;
            }
            is_sync = true;
        } else {
            *errs_uncorrected += errors - err_prev;
            for (size_t i = 0; i < 5; i++) {
                data[decodedBytes++] = 'X';
            }
            if (!is_sync) {
                *errs_total = errors;
                if (deco != 0)
                    goto end;
                return (RADIOLIB_ERR_MSG_CORRUPT);
            }
            is_sync = false;
            continue;
        }

        if (cw == RADIOLIB_PAGER_IDLE_CODE_WORD) {
            *complete = true;
            break;
        }

        if (cw == RADIOLIB_PAGER_FRAME_SYNC_CODE_WORD) {
            continue;
        }

        if (!(cw & (RADIOLIB_PAGER_MESSAGE_CODE_WORD << (RADIOLIB_PAGER_CODE_WORD_LEN - 1)))) {
            *addr_next = cw;
            break;
        }

        Serial.printf("RAW CW %X \n", cw);
        uint8_t bitPos = RADIOLIB_PAGER_CODE_WORD_LEN - 1 - symbolLength;
        if (overflow) {
            overflow = false;

            uint8_t currPos = RADIOLIB_PAGER_CODE_WORD_LEN - 1 - symbolLength + ovfBits;
            uint8_t prevPos = RADIOLIB_PAGER_MESSAGE_END_POS;
            uint32_t prevMask =
                    (0x7FUL << prevPos) & ~((uint32_t) 0x7FUL << (RADIOLIB_PAGER_MESSAGE_END_POS + ovfBits));
            uint32_t currMask = (0x7FUL << currPos) & ~((uint32_t) 1 << (RADIOLIB_PAGER_CODE_WORD_LEN - 1));

            uint8_t prevSymbol = (prevCw & prevMask) >> prevPos;
            uint8_t currSymbol = (cw & currMask) >> currPos;
            uint32_t symbol = prevSymbol << (symbolLength - ovfBits) | currSymbol;

            symbol = Module::reflect((uint8_t) symbol, 8);
            symbol >>= (8 - symbolLength);

            if (symbolLength == 4) {
                symbol = decodeBCD(symbol);
            }
            data[decodedBytes++] = symbol;
            deco++;

            bitPos += ovfBits;
            bitPos -= symbolLength;
        }

        while (bitPos >= RADIOLIB_PAGER_MESSAGE_END_POS) {
            uint32_t symbol = (cw & (0x7FUL << bitPos)) >> bitPos;
            symbol = Module::reflect((uint8_t) symbol, 8);
            symbol >>= (8 - symbolLength);

            if (symbolLength == 4) {
                symbol = decodeBCD(symbol);
            }
            data[decodedBytes++] = symbol;

            int8_t remBits = bitPos - RADIOLIB_PAGER_MESSAGE_END_POS;
            if (remBits < symbolLength) {
                prevCw = cw;
                overflow = true;
                ovfBits = remBits;
            }
            bitPos -= symbolLength;
        }

    }

    end:
    *len = decodedBytes;
    *errs_total = errors;
    return (RADIOLIB_ERR_NONE);
}

bool PagerClient::gotPreambleState() {
    if (phyLayer->gotPreamble) {
        phyLayer->gotPreamble = false;
        phyLayer->preambleBuffer = 0;
        return true;
    } else
        return false;
}

bool PagerClient::gotCarrierState() {
    if (phyLayer->gotCarrier) {
        phyLayer->gotCarrier = false;
        phyLayer->carrierBuffer = 0x9877FA3CA50B1DBD;
        return true;
    } else
        return false;
}

int16_t PagerClient::changeFreq(float base) {
    baseFreq = base;
    baseFreqRaw = (baseFreq * 1000000.0) / phyLayer->getFreqStep();

    int16_t state = phyLayer->setFrequency(baseFreq);
    RADIOLIB_ASSERT(state)

    state = phyLayer->receiveDirect();
    RADIOLIB_ASSERT(state)

    return (state);
}
