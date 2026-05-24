#pragma once

/*
 * This sample program only supports SX1276
 * */

// #define LILYGO_TBeam_V0_7
// #define LILYGO_TBeam_V1_X
// #define LILYGO_T3_V1_0
// #define LILYGO_T3_V1_3
//#define LILYGO_T3_V1_6
// #define LILYGO_T3_V2_0
// #define LILYGO_T3_S3_V1_0

/*
 * The default program uses 868MHz,
 * if you need to change it,
 * please open this note and change to the frequency you need to test
 * */

// #define LoRa_frequency      915.0

#define UNUSE_PIN                   (0)

#if defined(LILYGO_TBeam_V0_7)
#define GPS_RX_PIN                  12
#define GPS_TX_PIN                  15
#define BUTTON_PIN                  39
#define BUTTON_PIN_MASK             GPIO_SEL_39
#define I2C_SDA                     21
#define I2C_SCL                     22

#define RADIO_SCLK_PIN               5
#define RADIO_MISO_PIN              19
#define RADIO_MOSI_PIN              27
#define RADIO_CS_PIN                18
#define RADIO_DIO0_PIN               26
#define RADIO_RST_PIN               23
#define RADIO_DIO1_PIN              33
#define RADIO_BUSY_PIN              32

#define BOARD_LED                   14
#define LED_ON                      HIGH
#define LED_OFF                     LOW

#define GPS_BAUD_RATE               9600
#define HAS_GPS
#define HAS_DISPLAY

#elif defined(LILYGO_TBeam_V1_X)

#define GPS_RX_PIN                  34
#define GPS_TX_PIN                  12
#define BUTTON_PIN                  38
#define BUTTON_PIN_MASK             GPIO_SEL_38
#define I2C_SDA                     21
#define I2C_SCL                     22
#define PMU_IRQ                     35

#define RADIO_SCLK_PIN               5
#define RADIO_MISO_PIN              19
#define RADIO_MOSI_PIN              27
#define RADIO_CS_PIN                18
#define RADIO_DIO0_PIN               26
#define RADIO_RST_PIN               23
#define RADIO_DIO1_PIN              33
#define RADIO_BUSY_PIN              32

#define BOARD_LED                   4
#define LED_ON                      LOW
#define LED_OFF                     HIGH

#define GPS_BAUD_RATE               9600
#define HAS_GPS
#define HAS_DISPLAY
#define HAS_PMU

#elif defined(LILYGO_T3_V1_0)
#define I2C_SDA                     4
#define I2C_SCL                     15
#define OLED_RST                    16

#define RADIO_SCLK_PIN              5
#define RADIO_MISO_PIN              19
#define RADIO_MOSI_PIN              27
#define RADIO_CS_PIN                18
#define RADIO_DIO0_PIN               26
#define RADIO_RST_PIN               14
#define RADIO_DIO1_PIN              33
#define RADIO_BUSY_PIN              32

#define HAS_DISPLAY

#elif defined(LILYGO_T3_V1_3)

#define I2C_SDA                     21
#define I2C_SCL                     22
#define OLED_RST                    UNUSE_PIN

#define RADIO_SCLK_PIN              5
#define RADIO_MISO_PIN              19
#define RADIO_MOSI_PIN              27
#define RADIO_CS_PIN                18
#define RADIO_DIO0_PIN               26
#define RADIO_RST_PIN               14
#define RADIO_DIO1_PIN              33
#define RADIO_BUSY_PIN              32

#define ADC_PIN                     35

#define HAS_DISPLAY

#elif defined(LILYGO_T3_V1_6)
#define I2C_SDA                     21
#define I2C_SCL                     22
#define OLED_RST                    UNUSE_PIN

#define RADIO_SCLK_PIN              5
#define RADIO_MISO_PIN              19
#define RADIO_MOSI_PIN              27
#define RADIO_CS_PIN                18
#define RADIO_DIO0_PIN               26
#define RADIO_RST_PIN               23
#define RADIO_DIO1_PIN              33
#define RADIO_BUSY_PIN              32

#define SDCARD_MOSI                 15
#define SDCARD_MISO                 2
#define SDCARD_SCLK                 14
#define SDCARD_CS                   13

#define BOARD_LED                   25
#define LED_ON                      HIGH
#define LED_OFF                     LOW

#define ADC_PIN                     35

#define HAS_SDCARD
#define HAS_DISPLAY
#define FONT_12_GB2312 u8g2_font_wqy12_t_gb2312
#define OLED_TIMEOUT 60000 // ms
// #define HAS_OLED_TIMEOUT
// #define HAS_RTC

#define INITIAL_PPM                 6
#define AFC_ENABLE                  true
#define USE_SMARTCONFIG

#elif defined(LILYGO_T3_V2_0)
#define I2C_SDA                     21
#define I2C_SCL                     22
#define OLED_RST                    UNUSE_PIN

#define RADIO_SCLK_PIN              5
#define RADIO_MISO_PIN              19
#define RADIO_MOSI_PIN              27
#define RADIO_CS_PIN                18
#define RADIO_DIO0_PIN               26
#define RADIO_RST_PIN               14
#define RADIO_DIO1_PIN              UNUSE_PIN
#define RADIO_BUSY_PIN              UNUSE_PIN

#define SDCARD_MOSI                 15
#define SDCARD_MISO                 2
#define SDCARD_SCLK                 14
#define SDCARD_CS                   13

#define BOARD_LED                   0
#define LED_ON                      LOW

#define HAS_DISPLAY
#define HAS_SDCARD

#elif defined(LILYGO_T3_S3_V1_0) || defined(LILYGO_T3_S3_V1_2)

#define I2C_SDA                     18
#define I2C_SCL                     17
#define OLED_RST                    UNUSE_PIN

#define RADIO_SCLK_PIN              5
#define RADIO_MISO_PIN              3
#define RADIO_MOSI_PIN              6
#define RADIO_CS_PIN                7
#define RADIO_DIO1_PIN              33
#define RADIO_BUSY_PIN              34
#define RADIO_RST_PIN               8

//!SX1276/78 module only
#define RADIO_DIO0_PIN              9
#define RADIO_DIO3_PIN              21
#define RADIO_DIO4_PIN              10
#define RADIO_DIO5_PIN              36
//! end

#define SDCARD_MOSI                 11
#define SDCARD_MISO                 2
#define SDCARD_SCLK                 14
#define SDCARD_CS                   13

#define BOARD_LED                   37
#define LED_ON                      HIGH

#define BAT_ADC_PIN                1
#define BUTTON_PIN                 0

#define HAS_SDCARD
#define HAS_DISPLAY

#else
#error "For the first use, please define the board version and model in <config.h>"
#endif

/* ------------------------------------------------ */
/* Application constants                             */
/* ------------------------------------------------ */

// Radio
#define TARGET_FREQ 821.2375 // MHz

// POCSAG addresses
#define LBJ_INFO_ADDR 1234000
#define LBJ_INFO2_ADDR 1234002
#define LBJ_SYNC_ADDR 1234008

#define FUNCTION_DOWN 1
#define FUNCTION_UP 3

#define POCDAT_SIZE 16

// Timing
#define NETWORK_TIMEOUT 1800000 // 30 minutes
#define WDT_TIMEOUT 20          // sec
#define FD_TASK_STACK_SIZE 3000
#define FD_TASK_TIMEOUT 750     // ms
#define FD_TASK_ATTEMPTS 3
#define LED_ON_TIME 200         // ms

// SD log
#define MAX_LOG_SIZE 500000
#define MAX_CSV_SIZE 500000
#define LOG_VERBOSITY 0

/* ------------------------------------------------ */
/* Shared data structures                            */
/* ------------------------------------------------ */

#include <RadioLib.h>
#include <Arduino.h>

struct lbj_data {
    int8_t type = -1;
    char train[6] = "<NUL>";
    int8_t direction = -1;
    char speed[6] = "NUL";
    char position[7] = " <NUL>";
    char time[6] = "<NUL>";
    String info2_hex;
    String loco_type;
    char lbj_class[3] = "NA";
    char loco[9] = "<NUL>";
    char route[17] = "********";
    char route_utf8[17 * 2] = "********";
    char pos_lon_deg[4] = "";
    char pos_lon_min[8] = "";
    char pos_lat_deg[3] = "";
    char pos_lat_min[8] = "";
    char pos_lon[10] = "<NUL>";
    char pos_lat[9] = "<NUL>";
};

struct rx_info {
    float rssi = 0;
    float fer = 0;
    float ppm = 0;
    uint16_t cnt = 0;
    uint64_t timer = 0;
};

struct data_bond {
    PagerClient::pocsag_data pocsagData[POCDAT_SIZE];
    lbj_data lbjData;
    String str;
};

enum task_states {
    TASK_INIT = 0,
    TASK_CREATED = 1,
    TASK_RUNNING = 2,
    TASK_DONE = 3,
    TASK_TERMINATED = 4,
    TASK_CREATE_FAILED = 5,
    TASK_RUNNING_SCREEN = 6
};
