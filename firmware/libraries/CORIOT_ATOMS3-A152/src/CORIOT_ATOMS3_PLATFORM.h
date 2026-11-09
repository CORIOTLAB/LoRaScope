//==============================================================================
// CORIOT_ATOMS3_PLATFORM.h
//==============================================================================
// Arduino platform adapter
// Keep MCU-specific serial and task details in this file.
//==============================================================================
#ifndef CORIOT_ATOMS3_PLATFORM_H
#define CORIOT_ATOMS3_PLATFORM_H
//==============================================================================
#include <Arduino.h>
//==============================================================================
#ifndef CORIOT_SERIAL_TYPE
  #define CORIOT_SERIAL_TYPE HardwareSerial
#endif
//------------------------------------------------------------------------------
#if defined(ESP32)
  #include "freertos/FreeRTOS.h"
  #include "freertos/semphr.h"
  #include "freertos/task.h"
  #define CORIOT_SERIAL_BEGIN(SER,BAUD,RXPIN,TXPIN) \
          (SER).begin((BAUD),SERIAL_8N1,(RXPIN),(TXPIN))
#else
  // Most Arduino cores select UART pins outside Serial.begin().
  #define CORIOT_SERIAL_BEGIN(SER,BAUD,RXPIN,TXPIN) \
          do { (void)(RXPIN); (void)(TXPIN); (SER).begin((BAUD)); } while(0)
#endif
//==============================================================================
typedef void (*CORIOT_TASK_FN)(void *);
#if defined(ESP32)
typedef TaskHandle_t CORIOT_TASK_HANDLE;
typedef SemaphoreHandle_t CORIOT_MUTEX_HANDLE;
#else
typedef void *CORIOT_TASK_HANDLE;
typedef void *CORIOT_MUTEX_HANDLE;
#endif
//------------------------------------------------------------------------------
static inline CORIOT_MUTEX_HANDLE CORIOT_MUTEX_CREATE()
  {
  #if defined(ESP32)
  return xSemaphoreCreateMutex();
  #else
  return (CORIOT_MUTEX_HANDLE)1;
  #endif
  }
//------------------------------------------------------------------------------
static inline bool CORIOT_MUTEX_LOCK(CORIOT_MUTEX_HANDLE MUTEX,
                                     uint32_t WAIT_MS=1000UL)
  {
  #if defined(ESP32)
  if(!MUTEX) return false;
  return xSemaphoreTake(MUTEX,pdMS_TO_TICKS(WAIT_MS))==pdTRUE;
  #else
  (void)MUTEX;
  (void)WAIT_MS;
  return true;
  #endif
  }
//------------------------------------------------------------------------------
static inline void CORIOT_MUTEX_UNLOCK(CORIOT_MUTEX_HANDLE MUTEX)
  {
  #if defined(ESP32)
  if(MUTEX) xSemaphoreGive(MUTEX);
  #else
  (void)MUTEX;
  #endif
  }
//------------------------------------------------------------------------------
static inline bool CORIOT_TASK_START(CORIOT_TASK_FN FUNC,const char *NAME,
                                     uint32_t STACK,void *ARG,uint8_t PRIO)
  {
  #if defined(ESP32)
  return xTaskCreate(FUNC,NAME,STACK,ARG,PRIO,nullptr)==pdPASS;
  #else
  (void)FUNC;
  (void)NAME;
  (void)STACK;
  (void)ARG;
  (void)PRIO;
  return false;
  #endif
  }
//------------------------------------------------------------------------------
static inline void CORIOT_TASK_WAIT(uint32_t WAIT_MS)
  {
  #if defined(ESP32)
  vTaskDelay(pdMS_TO_TICKS(WAIT_MS));
  #else
  delay(WAIT_MS);
  #endif
  }
//------------------------------------------------------------------------------
static inline bool CORIOT_TASK_START_CORE(CORIOT_TASK_FN FUNC,const char *NAME,
                                          uint32_t STACK,void *ARG,uint8_t PRIO,
                                          CORIOT_TASK_HANDLE *HAND,uint8_t CORE)
  {
  #if defined(ESP32)
  return xTaskCreatePinnedToCore(FUNC,NAME,STACK,ARG,PRIO,HAND,CORE)==pdPASS;
  #else
  (void)FUNC;
  (void)NAME;
  (void)STACK;
  (void)ARG;
  (void)PRIO;
  (void)HAND;
  (void)CORE;
  return false;
  #endif
  }
//==============================================================================
#endif
//==============================================================================
