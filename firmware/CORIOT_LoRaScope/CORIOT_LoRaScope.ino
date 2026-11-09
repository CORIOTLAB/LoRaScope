//==============================================================================
// CORIOT_LoRaScope
// Board : M5Stack AtomS3
// Radio : A152 / RAK3172
// GNSS  : Simulation, u-blox M10 or M5 ATGM336H
//==============================================================================
#include <M5Unified.h>
#include "A152_CONFIG.h"
#include <CORIOT_ATOMS3_PLATFORM.h>
#include <CORIOT_general.h>
#include <CORIOT_ATGM336H.h>
#include <CORIOT_M10.h>
#include <CORIOT_NMEA_SERVICE.h>
#include <CORIOT_NMEA_SIM.h>
#include <CORIOT_ADXL345.h>
#include "CORIOT_LoRaScope_AUX.h"
//==============================================================================
#define GNSS_SRC_SIM   0
#define GNSS_SRC_M10   1
#define GNSS_SRC_ATGM  2
#ifndef GNSS_SOURCE
  #define GNSS_SOURCE GNSS_SRC_ATGM
#endif
#if GNSS_SOURCE==GNSS_SRC_SIM
  #define GNSS_NAME "SIMU"
#elif GNSS_SOURCE==GNSS_SRC_M10
  #define GNSS_NAME "M010"
#else
  #define GNSS_NAME "ATGM"
#endif
//==============================================================================
#ifndef LW_UPLINK_SEC
  #define LW_UPLINK_SEC 60UL
#endif
#define LW_TX_POWER 0
//==============================================================================
static uint8_t FLG=F_PRN;
static char BUF[384];
static CORIOT_SERIAL_TYPE GPS_SER(BOARD_GNSS_UART);
static NMEA_DATA GBD;
static NMEA_DATA GBS;
static CORIOT_NMEA_SERVICE GNS;
static ADXL_DATA ADXL;
static A152_DEV A152;
static const A152_RADIO_CONFIG A152_CFG={
  LW_DEVEUI,LW_APPEUI,LW_APPKEY,LW_CHANNEL_MASK,
  LW_ADR,LW_DATA_RATE,LW_CONFIRMED
};
static char LINE[160];
static uint16_t LPTR=0;
static uint32_t TPRN=0;
static uint32_t TUP=0;
static uint32_t TDIS=0;
static const bool PRN_RAW=false;
//==============================================================================
void setup()
  {
  auto CFG=M5.config();
  M5.begin(CFG);
  Serial.begin(115200);
  M5.Display.clear();
  delay(500);

  NMEA_RESET(GBD);
  NMEA_RESET(GBS);
  A152_RESET(A152,FLG,nullptr);

  bool GOK=true;
  NMEA_UPDATE_FN GUPD=nullptr;
#if GNSS_SOURCE==GNSS_SRC_SIM
  NMEA_SIM_UPDATE(GBD);
  GUPD=NMEA_SIM_UPDATE;
#else
  CORIOT_SERIAL_BEGIN(GPS_SER,BOARD_GNSS_BAUD,BOARD_GNSS_RX,BOARD_GNSS_TX);
  #if GNSS_SOURCE==GNSS_SRC_M10
    GOK=M10_INIT(GPS_SER,BOARD_GNSS_BAUD).OK;
  #else
    GOK=ATGM_INIT(GPS_SER,BOARD_GNSS_BAUD).OK;
  #endif
#endif
  snprintf(BUF,sizeof(BUF),"%s.IN | %s\r\n",GNSS_NAME,GOK?"OK":"ER");
  LOG_WRITE(FLG,nullptr,BUF);

  bool GTK=NMEA_SERVICE_START(GNS,GPS_SER,LINE,LPTR,sizeof(LINE),
                              GBD,PRN_RAW,GUPD);
  snprintf(BUF,sizeof(BUF),"NMEA.TK | %s\r\n",GTK?"OK":"ER");
  LOG_WRITE(FLG,nullptr,BUF);

  bool IOK=ADXL_INIT(ADXL);
  snprintf(BUF,sizeof(BUF),"ADXL.IN | %s ID:%02X SDA:%d SCL:%d\r\n",
           IOK?"OK":"ER",(unsigned)ADXL.ID,ADXL_SDA,ADXL_SCL);
  LOG_WRITE(FLG,nullptr,BUF);

  A152_INIT(A152,&Serial2,BOARD_A152_RX,BOARD_A152_TX,A152_CFG);
  TPRN=millis();
  TUP=millis();
  TDIS=0;
  NMEA_SERVICE_SNAPSHOT(GNS,GBS);
  A152_DRAW_STATUS(A152,GBS);
  }
//==============================================================================
void loop()
  {
  M5.update();
  if(!A152.TASK_OK) A152_UPDATE(A152);
  if(!GNS.TASK_OK) NMEA_SERVICE_UPDATE(GNS);
  ADXL_UPDATE(ADXL);

  if((uint32_t)(millis()-TPRN)>=1000UL)
    {
    TPRN=millis();
    if(NMEA_SERVICE_SNAPSHOT(GNS,GBS))
      {
      NMEA_BUILD_DATA(GBS);
      snprintf(BUF,sizeof(BUF),"%s\r\n",GBS.DATA);
      LOG_WRITE(FLG,nullptr,BUF);
      }
    snprintf(BUF,sizeof(BUF),
             "ADXL.DT | OK:%u AGE:%lu X:%.3f Y:%.3f Z:%.3f PT:%.1f RL:%.1f P8:%d R8:%d NS:%lu ER:%lu\r\n",
             ADXL_FRESH(ADXL),
             ADXL.SAMPLE_MS?(unsigned long)(millis()-ADXL.SAMPLE_MS):0UL,
             (double)ADXL.X,(double)ADXL.Y,(double)ADXL.Z,
             (double)ADXL.PITCH,(double)ADXL.ROLL,
             (int)ADXL.PITCH8,(int)ADXL.ROLL8,
             (unsigned long)ADXL.SAMPLE_CO,(unsigned long)ADXL.ERROR_CO);
    LOG_WRITE(FLG,nullptr,BUF);
    }

  if((uint32_t)(millis()-TUP)>=((uint32_t)LW_UPLINK_SEC*1000UL))
    {
    TUP=millis();
    A152_SEND_GNSS_SERVICE(A152,GNS,GBS,ADXL,
                           LW_UPLINK_PORT,LW_TX_POWER);
    }

  if(M5.BtnA.wasReleased())
    {
    TUP=millis();
    A152_SEND_GNSS_SERVICE(A152,GNS,GBS,ADXL,
                           LW_UPLINK_PORT,LW_TX_POWER);
    }

  if((uint32_t)(millis()-TDIS)>=1000UL)
    {
    TDIS=millis();
    if(NMEA_SERVICE_SNAPSHOT(GNS,GBS)) A152_DRAW_STATUS(A152,GBS);
    }
  delay(2);
  }
//==============================================================================
