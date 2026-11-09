//==============================================================================
// Copy this file as A152_CONFIG.h and enter local ChirpStack credentials.
//==============================================================================
#ifndef A152_CONFIG_KEYS_H
#define A152_CONFIG_KEYS_H
//==============================================================================
#define A152_01_DEVEUI "0000000000000000"
#define A152_01_APPEUI "0000000000000000"
#define A152_01_APPKEY "00000000000000000000000000000000"
//------------------------------------------------------------------------------
#define A152_02_DEVEUI "0000000000000000"
#define A152_02_APPEUI "0000000000000000"
#define A152_02_APPKEY "00000000000000000000000000000000"
//------------------------------------------------------------------------------
#define A152_03_DEVEUI "0000000000000000"
#define A152_03_APPEUI "0000000000000000"
#define A152_03_APPKEY "00000000000000000000000000000000"
//------------------------------------------------------------------------------
#define A152_04_DEVEUI "0000000000000000"
#define A152_04_APPEUI "0000000000000000"
#define A152_04_APPKEY "00000000000000000000000000000000"
//==============================================================================
#endif
//==============================================================================

//==============================================================================
// AtomS3 wiring and active A152 LoRaWAN configuration
//==============================================================================
#ifndef A152_PROJECT_CONFIG_H
#define A152_PROJECT_CONFIG_H
//==============================================================================
#define BOARD_GNSS_UART  1
#define BOARD_GNSS_RX    1
#define BOARD_GNSS_TX    2
#define BOARD_GNSS_BAUD  115200UL
#define BOARD_A152_RX    6
#define BOARD_A152_TX    5
#define BOARD_ADXL_SDA   38
#define BOARD_ADXL_SCL   39
#define BOARD_ADXL_I2C   400000UL
//==============================================================================
#define A152_01 1
#define A152_02 2
#define A152_03 3
#define A152_04 4
#ifndef A152_KEYS
  #define A152_KEYS A152_02
#endif
#if A152_KEYS==A152_01
  #define A152_DEVICE_ID "01"
  #define LW_DEVEUI A152_01_DEVEUI
  #define LW_APPEUI A152_01_APPEUI
  #define LW_APPKEY A152_01_APPKEY
#elif A152_KEYS==A152_02
  #define A152_DEVICE_ID "02"
  #define LW_DEVEUI A152_02_DEVEUI
  #define LW_APPEUI A152_02_APPEUI
  #define LW_APPKEY A152_02_APPKEY
#elif A152_KEYS==A152_03
  #define A152_DEVICE_ID "03"
  #define LW_DEVEUI A152_03_DEVEUI
  #define LW_APPEUI A152_03_APPEUI
  #define LW_APPKEY A152_03_APPKEY
#elif A152_KEYS==A152_04
  #define A152_DEVICE_ID "04"
  #define LW_DEVEUI A152_04_DEVEUI
  #define LW_APPEUI A152_04_APPEUI
  #define LW_APPKEY A152_04_APPKEY
#else
  #error "Invalid A152_KEYS selection"
#endif
#define CORIOT_LOG_DEVICE_ID A152_DEVICE_ID
#define LW_CHANNEL_MASK "0002"
#define LW_UPLINK_PORT  1
#define LW_UPLINK_SEC   5UL
#define LW_CONFIRMED    true
#define LW_ADR          false
#define LW_DATA_RATE    5
//==============================================================================
#endif
//==============================================================================
