//==============================================================================
// CORIOT_general.h
//==============================================================================
#ifndef CORIOT_GENERAL_H
#define CORIOT_GENERAL_H
//------------------------------------------------------------------------------
#include <Wire.h>
#if defined(ESP32)
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif
//==============================================================================
// CORIOT_TIME - Return valid epoch time; zero means time is not synchronized
//==============================================================================
inline uint32_t CORIOT_TIME(uint32_t EPOCH=0)
  {
  return EPOCH>100000 ? EPOCH : 0;
  }
//==============================================================================
// COMPACT INTEGER ENCODING - Reusable one-byte telemetry conversions
//==============================================================================
inline int8_t CORIOT_INT8_ROUND(float VALUE,int8_t INVALID=-128,
                                bool VALID=true)
  {
  if(!VALID) return INVALID;
  int16_t OUT=(int16_t)(VALUE+(VALUE>=0.0f?0.5f:-0.5f));
  int16_t MINIMUM=INVALID==-128?-127:-128;
  if(OUT<MINIMUM) OUT=MINIMUM;
  if(OUT>127) OUT=127;
  return (int8_t)OUT;
  }
//------------------------------------------------------------------------------
inline uint8_t CORIOT_UINT8_ROUND(float VALUE,uint8_t MINIMUM=0,
                                  uint8_t MAXIMUM=254,uint8_t INVALID=255,
                                  bool VALID=true)
  {
  if(!VALID) return INVALID;
  int16_t OUT=(int16_t)(VALUE+0.5f);
  if(OUT<MINIMUM) OUT=MINIMUM;
  if(OUT>MAXIMUM) OUT=MAXIMUM;
  return (uint8_t)OUT;
  }
//==============================================================================
// I2C_SCAN - Scan I2C bus, fill array with detected devices, return count+string
//==============================================================================
inline uint8_t I2C_SCAN(TwoWire &WIRIF, bool I2CDEV[128],
                        char *OUTBUF, size_t BUFLEN,
                        const char *HEADER)
  {
//------------------------------------------------------------------------------
  uint8_t COUNT = 0;
  int POS = snprintf(OUTBUF, BUFLEN, "%s ", HEADER);
  for(uint8_t ADDRS = 1; ADDRS < 128; ADDRS++)
    {
    WIRIF.beginTransmission(ADDRS);
    uint8_t ERRCD = WIRIF.endTransmission();
    I2CDEV[ADDRS] = (ERRCD == 0);
    if(I2CDEV[ADDRS])
      {
      COUNT++;
      POS += snprintf(OUTBUF+POS, BUFLEN-POS, "0x%02X ", ADDRS);
      }
    }
  if(POS > 0 && POS < (int)BUFLEN) OUTBUF[POS-1] = '\0';
  return COUNT;
//------------------------------------------------------------------------------
  }
//==============================================================================
// FLAGS - Generic stream flags (reusable for many sensors)
//==============================================================================
// b0 ENA: enable stream
// b1 PRN: print serial
// b2 SD : save SD (only if CORIOT_HAS_SD is defined in sketch)
//==============================================================================
#define F_ENA  0x01
#define F_PRN  0x02
#define F_SD   0x04
//==============================================================================
// Flag helpers
//==============================================================================
inline bool F_IS(uint8_t F, uint8_t M) { return (F & M) != 0; }
inline void F_ON(uint8_t &F, uint8_t M) { F |= M; }
inline void F_OFF(uint8_t &F, uint8_t M) { F &= (uint8_t)(~M); }
//==============================================================================
// LOG_WRITE - Unified log output (Serial + optional SD)
//==============================================================================
inline void LOG_WRITE(uint8_t FLG,const char *PTH,const char *LINE)
  {
  const char *OUT=LINE;
  #if defined(CORIOT_LOG_DEVICE_ID)
  char TAGGED[512];
  const char *SEP=strstr(LINE," |");
  if(SEP)
    {
    size_t HEAD=(size_t)(SEP-LINE);
    int N=snprintf(TAGGED,sizeof(TAGGED),"%.*s-%s%s",
                   (int)HEAD,LINE,CORIOT_LOG_DEVICE_ID,SEP);
    if(N>0 && (size_t)N<sizeof(TAGGED)) OUT=TAGGED;
    }
  #endif
  if(F_IS(FLG,F_PRN)) Serial.print(OUT);
  #if defined(CORIOT_HAS_SD)
  if(F_IS(FLG,F_SD)) CORIOT::APPND(SD,PTH,OUT);
  #else
  (void)PTH;
  #endif
  }
//==============================================================================
// LOG_LINE - Unified log output (Serial + optional SD) using flags (expr-safe)
//==============================================================================
// - LINE must include "\r\n" if you want line breaks in file
// - Works in: LOG_LINE(...);   and in: snprintf(...),LOG_LINE(...);
//==============================================================================
#if defined(CORIOT_HAS_SD)
  #define LOG_LINE(FLG,PTH,LINE) \
    ([&]()->int{ if(F_IS((FLG),F_PRN)) Serial.print((LINE)); \
                if(F_IS((FLG),F_SD))  CORIOT::APPND(SD,(PTH),(LINE)); \
                return 0; })()
#else
  #define LOG_LINE(FLG,PTH,LINE) \
    ([&]()->int{ (void)(PTH); if(F_IS((FLG),F_PRN)) Serial.print((LINE)); \
                return 0; })()
#endif
//==============================================================================
// SYS - Dual-core load monitor + snapshot (ESP32)
//==============================================================================

#if defined(ESP32)
static volatile uint32_t _SYS_IDL0 = 0;
static volatile uint32_t _SYS_IDL1 = 0;
static uint32_t _SYS_P0 = 0;
static uint32_t _SYS_P1 = 0;
static uint32_t _SYS_T0 = 0;
static bool     _SYS_MON_OK = false;
static TaskHandle_t _SYS_TH0 = nullptr;
static TaskHandle_t _SYS_TH1 = nullptr;

inline void _SYS_IDLE_TASK(void *ARG)
  {
  uint32_t CORE = (uint32_t)(uintptr_t)ARG;
  for(;;)
    {
    if(CORE == 0) _SYS_IDL0++;
    else          _SYS_IDL1++;
    vTaskDelay(1);
    }
  }

inline bool SYS_MON_INIT(uint16_t STACK=1024, UBaseType_t PRIO=0)
  {
  if(_SYS_MON_OK) return true;
  BaseType_t A = xTaskCreatePinnedToCore(_SYS_IDLE_TASK,"sysIdle0",
                                        STACK,(void*)0,PRIO,&_SYS_TH0,0);
  BaseType_t B = xTaskCreatePinnedToCore(_SYS_IDLE_TASK,"sysIdle1",
                                        STACK,(void*)1,PRIO,&_SYS_TH1,1);
  _SYS_P0 = _SYS_IDL0;
  _SYS_P1 = _SYS_IDL1;
  _SYS_T0 = (uint32_t)millis();
  _SYS_MON_OK = (A == pdPASS) && (B == pdPASS);
  return _SYS_MON_OK;
  }
#endif

struct SYS_DATA
  {
  bool     VALID;
  uint32_t CPUMH;
  uint32_t COREI;
  uint32_t HEAPF;
  uint32_t HEAPM;
  uint32_t HEAPX;
  uint32_t PSRMF;
  uint32_t PSRMS;
  uint32_t STKBY;
  float    LOAD0;
  float    LOAD1;
  char     DATA[200];
  };

inline SYS_DATA SYS_SNAP(void)
  {
//------------------------------------------------------------------------------
  SYS_DATA OUT;
  OUT.VALID = false;
  OUT.CPUMH = 0; OUT.COREI = 0;
  OUT.HEAPF = OUT.HEAPM = OUT.HEAPX = 0;
  OUT.PSRMF = OUT.PSRMS = 0;
  OUT.STKBY = 0;
  OUT.LOAD0 = -1.0f; OUT.LOAD1 = -1.0f;
  OUT.DATA[0] = 0;
#if defined(ESP32)
  OUT.CPUMH = (uint32_t)ESP.getCpuFreqMHz();
  OUT.COREI = (uint32_t)xPortGetCoreID();
  OUT.HEAPF = (uint32_t)ESP.getFreeHeap();
  OUT.HEAPM = (uint32_t)ESP.getMinFreeHeap();
  OUT.HEAPX = (uint32_t)ESP.getMaxAllocHeap();
  #if defined(BOARD_HAS_PSRAM)
  OUT.PSRMF = (uint32_t)ESP.getFreePsram();
  OUT.PSRMS = (uint32_t)ESP.getPsramSize();
  #else
  OUT.PSRMF = 0;
  OUT.PSRMS = 0;
  #endif
  UBaseType_t STKW = uxTaskGetStackHighWaterMark(NULL);
  OUT.STKBY = (uint32_t)STKW * (uint32_t)sizeof(StackType_t);
  if(_SYS_MON_OK)
    {
    uint32_t now = (uint32_t)millis();
    uint32_t dt  = now - _SYS_T0;
    if(dt < 50) dt = 50;
    uint32_t c0 = _SYS_IDL0, c1 = _SYS_IDL1;
    uint32_t d0 = c0 - _SYS_P0;
    uint32_t d1 = c1 - _SYS_P1;
    _SYS_P0 = c0; _SYS_P1 = c1; _SYS_T0 = now;
    float r0 = (float)d0 / (float)dt;
    float r1 = (float)d1 / (float)dt;
    static float M0 = 0.0f, M1 = 0.0f;
    if(r0 > M0) M0 = r0;
    if(r1 > M1) M1 = r1;
    if(M0 > 0.0f) OUT.LOAD0 = 100.0f * (1.0f - (r0 / M0));
    if(M1 > 0.0f) OUT.LOAD1 = 100.0f * (1.0f - (r1 / M1));
    if(OUT.LOAD0 < 0) OUT.LOAD0 = 0; if(OUT.LOAD0 > 100) OUT.LOAD0 = 100;
    if(OUT.LOAD1 < 0) OUT.LOAD1 = 0; if(OUT.LOAD1 > 100) OUT.LOAD1 = 100;
    }
  snprintf(OUT.DATA, sizeof(OUT.DATA),
           "SYS1.D,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%.0f,%.0f",
           (unsigned long)OUT.CPUMH,
           (unsigned long)OUT.COREI,
           (unsigned long)OUT.HEAPF,
           (unsigned long)OUT.HEAPM,
           (unsigned long)OUT.HEAPX,
           (unsigned long)OUT.PSRMF,
           (unsigned long)OUT.PSRMS,
           (unsigned long)OUT.STKBY,
           (double)OUT.LOAD0,
           (double)OUT.LOAD1);
  OUT.VALID = true;
  return OUT;
#else
  snprintf(OUT.DATA, sizeof(OUT.DATA), "SYS.D,NA");
  return OUT;
#endif
//------------------------------------------------------------------------------
  }
//==============================================================================
#endif
//==============================================================================
