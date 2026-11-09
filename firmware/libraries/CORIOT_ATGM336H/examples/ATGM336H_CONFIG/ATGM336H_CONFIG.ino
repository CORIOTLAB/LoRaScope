//==============================================================================
// ATGM336H_CONFIG.ino - Configure and verify two ATGM336H on Teensy 4.1
//==============================================================================
#include <CORIOT_ATGM336H.h>
//==============================================================================
#define USB_BAUD 921600
#define ATGM_UART_BAUD 115200
#define ATGM_APPLY_CONFIG 1
#define VERIFY_MS 5000UL
//------------------------------------------------------------------------------
// Keep all signal-analysis information at the native 1 Hz positioning rate.
// Disable only navigation sentences whose contents are duplicated by GGA/RMC.
static const ATGM_NMEA_PROFILE SIGNAL_PROFILE={
  1, // GGA: fix quality, satellites used, HDOP, altitude
  0, // GLL: duplicated position/time
  1, // GSA: active satellites and PDOP/HDOP/VDOP
  1, // GSV: visible satellites, elevation, azimuth and C/N0
  1, // RMC: UTC date/time, validity, position, speed and course
  0, // VTG: duplicated speed/course
  0, // ZDA: duplicated UTC date/time
  1  // TXT: receiver diagnostic text/events
  };
//------------------------------------------------------------------------------
struct NMEA_INVENTORY
  {
  uint32_t GGA,GLL,GSA,GSV,RMC,VTG,ZDA,TXT,OTHER,BAD;
  char LINE[192];
  size_t INDEX;
  bool SYNCED;
  };
//------------------------------------------------------------------------------
static void INVENTORY_RESET(NMEA_INVENTORY &I)
  {
  memset(&I,0,sizeof(I));
  }
//------------------------------------------------------------------------------
static bool NMEA_CHECKSUM_OK(const char *LINE)
  {
  if(!LINE || LINE[0]!='$') return false;
  const char *STAR=strchr(LINE,'*');
  if(!STAR || !STAR[1] || !STAR[2]) return false;
  uint8_t CS=0;
  for(const char *P=LINE+1;P<STAR;P++) CS^=(uint8_t)*P;
  char CS_TEXT[3]={STAR[1],STAR[2],0};
  return CS==(uint8_t)strtoul(CS_TEXT,nullptr,16);
  }
//------------------------------------------------------------------------------
static void INVENTORY_LINE(NMEA_INVENTORY &I,const char *LINE,const char *TAG)
  {
  if(!NMEA_CHECKSUM_OK(LINE)) { I.BAD++; return; }
  const char *COMMA=strchr(LINE,',');
  if(!COMMA || COMMA-LINE<4) { I.OTHER++; return; }
  char TYPE[4]={COMMA[-3],COMMA[-2],COMMA[-1],0};
  if(!strcmp(TYPE,"GGA")) I.GGA++;
  else if(!strcmp(TYPE,"GLL")) I.GLL++;
  else if(!strcmp(TYPE,"GSA")) I.GSA++;
  else if(!strcmp(TYPE,"GSV")) I.GSV++;
  else if(!strcmp(TYPE,"RMC")) I.RMC++;
  else if(!strcmp(TYPE,"VTG")) I.VTG++;
  else if(!strcmp(TYPE,"ZDA")) I.ZDA++;
  else if(!strcmp(TYPE,"TXT")) I.TXT++;
  else I.OTHER++;
  Serial.print(TAG); Serial.print(" | "); Serial.println(LINE);
  }
//------------------------------------------------------------------------------
static void INVENTORY_FEED(Stream &PORT,NMEA_INVENTORY &I,const char *TAG)
  {
  while(PORT.available())
    {
    char C=(char)PORT.read();
    if(C=='\r') continue;
    if(C=='\n')
      {
      if(!I.SYNCED) { I.SYNCED=true; I.INDEX=0; continue; }
      if(I.INDEX) { I.LINE[I.INDEX]=0; INVENTORY_LINE(I,I.LINE,TAG); }
      I.INDEX=0;
      continue;
      }
    if(I.INDEX<sizeof(I.LINE)-1) I.LINE[I.INDEX++]=C;
    else I.INDEX=0;
    }
  }
//------------------------------------------------------------------------------
static bool INVENTORY_OK(const NMEA_INVENTORY &I)
  {
  return I.GGA && I.GSA && I.GSV && I.RMC &&
         !I.GLL && !I.VTG && !I.ZDA && !I.BAD;
  }
//------------------------------------------------------------------------------
static void INVENTORY_PRINT(const char *TAG,const NMEA_INVENTORY &I)
  {
  Serial.printf("%s.C | GGA:%lu GLL:%lu GSA:%lu GSV:%lu RMC:%lu VTG:%lu ZDA:%lu TXT:%lu OTHER:%lu BAD:%lu OK:%u\n",
                TAG,I.GGA,I.GLL,I.GSA,I.GSV,I.RMC,I.VTG,I.ZDA,I.TXT,
                I.OTHER,I.BAD,INVENTORY_OK(I));
  }
//==============================================================================
void setup()
  {
  Serial.begin(USB_BAUD);
  Serial1.begin(ATGM_UART_BAUD);
  Serial2.begin(ATGM_UART_BAUD);
  delay(3000);
  Serial.println("ATGM.I | START S1:SERIAL1 S2:SERIAL2 BAUD:115200");

  // Firmware/hardware queries are echoed with the normal receiver output.
  ATGM_QUERY_INFO(Serial1,0); ATGM_QUERY_INFO(Serial2,0);
  ATGM_QUERY_INFO(Serial1,1); ATGM_QUERY_INFO(Serial2,1);
  delay(500);

#if ATGM_APPLY_CONFIG
  bool TX1=ATGM_SET_NMEA(Serial1,SIGNAL_PROFILE);
  bool TX2=ATGM_SET_NMEA(Serial2,SIGNAL_PROFILE);
  Serial.printf("ATGM.W | S1:%u S2:%u CMD:$PCAS03,1,0,1,1,1,0,0,1*03\n",TX1,TX2);
#else
  Serial.println("ATGM.W | DISABLED");
#endif

  NMEA_INVENTORY S1,S2;
  INVENTORY_RESET(S1); INVENTORY_RESET(S2);
  uint32_t T0=millis();
  while((uint32_t)(millis()-T0)<VERIFY_MS)
    {
    INVENTORY_FEED(Serial1,S1,"S1");
    INVENTORY_FEED(Serial2,S2,"S2");
    }
  INVENTORY_PRINT("S1",S1);
  INVENTORY_PRINT("S2",S2);

#if ATGM_APPLY_CONFIG
  if(TX1 && TX2 && INVENTORY_OK(S1) && INVENTORY_OK(S2))
    {
    bool SAVE1=ATGM_SAVE_CONFIG(Serial1);
    bool SAVE2=ATGM_SAVE_CONFIG(Serial2);
    Serial.printf("ATGM.N | SAVE S1:%u S2:%u CMD:$PCAS00*01\n",SAVE1,SAVE2);
    }
  else Serial.println("ATGM.N | NOT_SAVED VALIDATION_FAILED");
#endif
  Serial.println("ATGM.E | USB ECHO; reset or power-cycle to verify persistence");
  }
//==============================================================================
void loop()
  {
  while(Serial1.available()) Serial.write(Serial1.read());
  while(Serial2.available()) Serial.write(Serial2.read());
  }
//==============================================================================
