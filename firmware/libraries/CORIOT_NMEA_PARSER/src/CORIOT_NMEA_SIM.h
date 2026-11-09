//==============================================================================
// CORIOT_NMEA_SIM.h
//==============================================================================
#ifndef CORIOT_NMEA_SIM_H
#define CORIOT_NMEA_SIM_H
//==============================================================================
#include <CORIOT_NMEA_PARSER.h>
//==============================================================================
static inline void NMEA_SIM_UPDATE(NMEA_DATA &D)
  {
  uint32_t STEP=(millis()/1000UL)%20UL;
  D.POS_OK=true;
  D.RMC_OK=true;
  D.FIX_Q=1;
  D.FIX_TP=3;
  D.GPS_VIS=9;
  D.BDS_VIS=5;
  D.GLO_VIS=4;
  D.GAL_VIS=6;
  D.SAT_USED=12;
  D.PDOP=1.4f;
  D.HDOP=0.8f;
  D.VDOP=1.1f;
  D.LAT=6.244203+(double)STEP*0.000001;
  D.LON=-75.581211-(double)STEP*0.000001;
  D.ALT=1495.2;
  D.RX_MS=D.POS_MS=D.EPOCH_MS=millis();
  D._EPOCH_MASK=0x03;
  D._GSV_COMPLETE=0x0F;
  D.SAT_VALID=true;
  D.EPOCH_VALID=true;
  }
//==============================================================================
#endif
//==============================================================================
