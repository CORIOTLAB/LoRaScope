//==============================================================================
// CORIOT_LoRaScope_AUX.h
//==============================================================================
#ifndef CORIOT_LORASCOPE_AUX_H
#define CORIOT_LORASCOPE_AUX_H
//------------------------------------------------------------------------------
#include <CORIOT_A152_RADIO.h>
#include <CORIOT_ATOMS3_DISPLAY.h>
#include <CORIOT_NMEA_PARSER.h>
#include <CORIOT_ADXL345.h>
#include <CORIOT_general.h>
#include <CORIOT_NMEA_SERVICE.h>
//==============================================================================
static inline bool A152_SEND_GNSS_VERIFIED(A152_DEV &DEV,
                                           const NMEA_DATA &GBD,
                                           const ADXL_DATA &ADXL,
                                           uint8_t PORT,uint8_t TX_POWER)
  {
  if(!DEV.JOIN_OK)
    {
    LOG_WRITE(DEV.FLG,DEV.PTH,"A152.UP | ER WAIT_JOIN\r\n");
    return false;
    }
  if(!GBD.EPOCH_VALID)
    {
    LOG_WRITE(DEV.FLG,DEV.PTH,"GNBS.UP | ER INVALID_EPOCH\r\n");
    return false;
    }
  if(DEV.TX_POWER!=TX_POWER)
    {
    A152_LOGF("A152.TP | ER STALE:%u REQ:%u\r\n",
              (unsigned)DEV.TX_POWER,(unsigned)TX_POWER);
    return false;
    }
  uint8_t PAY[NMEA_PAY17_LEN+3];
  NMEA_PACK17(PAY,GBD);
  bool AOK=ADXL_FRESH(ADXL);
  PAY[NMEA_PAY17_LEN]=(uint8_t)(AOK?ADXL.PITCH8:0);
  PAY[NMEA_PAY17_LEN+1]=(uint8_t)(AOK?ADXL.ROLL8:0);
  PAY[NMEA_PAY17_LEN+2]=0x20|(DEV.TX_POWER&0x0F);
  bool OK=A152_SEND_BIN(DEV,PAY,sizeof(PAY),PORT);
  if(OK)
    {
    char OUT[256];
    snprintf(OUT,sizeof(OUT),
             "GNBS.TX | LAT:%.6f LON:%.6f ALT:%.1f SAT:%u AGE:%lu PT:%.1f RL:%.1f TP:%u UC:%lu AC:%lu RX:%lu\r\n",
             GBD.LAT,GBD.LON,GBD.ALT,(unsigned)GBD.SAT_USED,
             (unsigned long)(millis()-GBD.POS_MS),
             (double)ADXL.PITCH,(double)ADXL.ROLL,(unsigned)DEV.TX_POWER,
             (unsigned long)DEV.UP_CO,(unsigned long)DEV.ACK_CO,
             (unsigned long)DEV.RX_CO);
    LOG_WRITE(DEV.FLG,DEV.PTH,OUT);
    }
  return OK;
  }
//------------------------------------------------------------------------------
static inline bool A152_SEND_GNSS_SERVICE(A152_DEV &DEV,
                                          CORIOT_NMEA_SERVICE &SERVICE,
                                          NMEA_DATA &SNAPSHOT,
                                          ADXL_DATA &ADXL,
                                          uint8_t PORT,uint8_t TX_POWER)
  {
  if(!NMEA_SERVICE_SNAPSHOT(SERVICE,SNAPSHOT))
    {
    LOG_WRITE(DEV.FLG,DEV.PTH,"GNBS.UP | ER SNAPSHOT\r\n");
    return false;
    }
  if(!SNAPSHOT.EPOCH_VALID)
    {
    LOG_WRITE(DEV.FLG,DEV.PTH,"GNBS.UP | ER NO_FIX\r\n");
    return false;
    }
  if(!A152_VERIFY_TX_POWER(DEV,TX_POWER)) return false;
  if(!NMEA_SERVICE_SNAPSHOT(SERVICE,SNAPSHOT))
    {
    LOG_WRITE(DEV.FLG,DEV.PTH,"GNBS.UP | ER SNAPSHOT\r\n");
    return false;
    }
  if(!SNAPSHOT.EPOCH_VALID)
    {
    LOG_WRITE(DEV.FLG,DEV.PTH,"GNBS.UP | ER FIX_LOST\r\n");
    return false;
    }
  ADXL_UPDATE(ADXL);
  return A152_SEND_GNSS_VERIFIED(DEV,SNAPSHOT,ADXL,PORT,TX_POWER);
  }
//==============================================================================
static inline void A152_DRAW_STATUS(A152_DEV &DEV,const NMEA_DATA &GBD)
  {
  static uint32_t UP_CO=0;
  static uint32_t ACK_CO=0;
  A152_GET_COUNTERS(DEV,UP_CO,ACK_CO);
  ATOMS3_DRAW_GNSS_STATUS(GBD,UP_CO,ACK_CO);
  }
//==============================================================================
#endif
//==============================================================================
