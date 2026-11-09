//==============================================================================
// CORIOT_ATGM336H.h
//==============================================================================
// CASIC configuration driver for the M5 ATGM336H GNSS unit
// NMEA parsing is handled by CORIOT_NMEA_PARSER.h
//==============================================================================
#ifndef CORIOT_ATGM336H_H
#define CORIOT_ATGM336H_H
//==============================================================================
#include <Arduino.h>
//==============================================================================
#ifndef ATGM_BAUD
  #define ATGM_BAUD 115200UL
#endif
#ifndef ATGM_SYSTEM_MASK
  #define ATGM_SYSTEM_MASK 15
#endif
#ifndef ATGM_BOOT_MS
  #define ATGM_BOOT_MS 250UL
#endif
//==============================================================================
struct ATGM_INRS
  {
  bool OK;
  char CFG[96];
  };
//------------------------------------------------------------------------------
struct ATGM_NMEA_PROFILE
  {
  uint8_t GGA;
  uint8_t GLL;
  uint8_t GSA;
  uint8_t GSV;
  uint8_t RMC;
  uint8_t VTG;
  uint8_t ZDA;
  uint8_t TXT;
  };
//==============================================================================
static inline uint8_t _ATGM_CHECKSUM(const char *BODY)
  {
  uint8_t CS=0;
  while(BODY && *BODY) CS^=(uint8_t)*BODY++;
  return CS;
  }
//------------------------------------------------------------------------------
static inline bool ATGM_SEND_CASIC(Stream &SERIF,const char *BODY)
  {
  if(!BODY || !BODY[0]) return false;
  char CMD[80];
  int N=snprintf(CMD,sizeof(CMD),"$%s*%02X\r\n",BODY,_ATGM_CHECKSUM(BODY));
  if(N<=0 || N>=(int)sizeof(CMD)) return false;
  return SERIF.write((const uint8_t*)CMD,(size_t)N)==(size_t)N;
  }
//------------------------------------------------------------------------------
static inline bool ATGM_SET_SYSTEMS(Stream &SERIF,uint8_t MASK=ATGM_SYSTEM_MASK)
  {
  char BODY[24];
  snprintf(BODY,sizeof(BODY),"PCAS04,%u",(unsigned)MASK);
  return ATGM_SEND_CASIC(SERIF,BODY);
  }
//------------------------------------------------------------------------------
// Classic AT6558/ATGM336H PCAS03 layout:
// GGA,GLL,GSA,GSV,RMC,VTG,ZDA,TXT. Each value is 0 (off) or 1..9
// (output once every N positioning updates).
static inline bool ATGM_SET_NMEA(Stream &SERIF,const ATGM_NMEA_PROFILE &PROFILE)
  {
  const uint8_t *VALUE=&PROFILE.GGA;
  for(uint8_t I=0;I<8;I++) if(VALUE[I]>9) return false;
  char BODY[48];
  snprintf(BODY,sizeof(BODY),"PCAS03,%u,%u,%u,%u,%u,%u,%u,%u",
           PROFILE.GGA,PROFILE.GLL,PROFILE.GSA,PROFILE.GSV,
           PROFILE.RMC,PROFILE.VTG,PROFILE.ZDA,PROFILE.TXT);
  return ATGM_SEND_CASIC(SERIF,BODY);
  }
//------------------------------------------------------------------------------
static inline bool ATGM_QUERY_INFO(Stream &SERIF,uint8_t INFO=0)
  {
  if(INFO>5 || INFO==4) return false;
  char BODY[20];
  snprintf(BODY,sizeof(BODY),"PCAS06,%u",INFO);
  return ATGM_SEND_CASIC(SERIF,BODY);
  }
//------------------------------------------------------------------------------
static inline bool ATGM_SAVE_CONFIG(Stream &SERIF)
  {
  return ATGM_SEND_CASIC(SERIF,"PCAS00");
  }
//------------------------------------------------------------------------------
static inline bool ATGM_WAIT_NMEA(Stream &SERIF,uint32_t WAIT_MS=1200UL)
  {
  uint32_t T0=millis();
  bool START=false;
  while((uint32_t)(millis()-T0)<WAIT_MS)
    {
    while(SERIF.available())
      {
      char C=(char)SERIF.read();
      if(C=='$') START=true;
      else if(START && C=='*') return true;
      else if(C=='\n' || C=='\r') START=false;
      }
    delay(1);
    }
  return false;
  }
//==============================================================================
static inline ATGM_INRS ATGM_INIT(Stream &SERIF,
                                  uint32_t BAUD=ATGM_BAUD)
  {
  ATGM_INRS R;
  R.OK=false;
  R.CFG[0]='\0';
  (void)BAUD;
  delay(ATGM_BOOT_MS);
  while(SERIF.available()) SERIF.read();
  bool WRITE_OK=ATGM_SET_SYSTEMS(SERIF,ATGM_SYSTEM_MASK);
  bool NMEA_OK=ATGM_WAIT_NMEA(SERIF);
  R.OK=WRITE_OK&&NMEA_OK;
  snprintf(R.CFG,sizeof(R.CFG),"BAUD:%lu SYS:0x%02X CMD:%u NMEA:%u",
           (unsigned long)BAUD,(unsigned)ATGM_SYSTEM_MASK,
           WRITE_OK,NMEA_OK);
  return R;
  }
//==============================================================================
#endif
//==============================================================================
