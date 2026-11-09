//==============================================================================
// CORIOT_NMEA_PARSER.h
//==============================================================================
// Basic multi-brand NMEA parser for mapper use
// Focus:
// - accepted position / fix state / DOP
// - visible satellites by constellation
// - used satellites by constellation
// - total satellites used
// - simple CN0 summary by constellation
//==============================================================================
#ifndef CORIOT_NMEA_PARSER_H
#define CORIOT_NMEA_PARSER_H
//==============================================================================
#include <Arduino.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
//==============================================================================
#define NMEA_GPS 0
#define NMEA_GLO 1
#define NMEA_GAL 2
#define NMEA_BDS 3
#define NMEA_SYSN 4
#define NMEA_PAY17_LEN 17
#define NMEA_FIX_MAX_AGE_MS 2500UL
//==============================================================================
struct NMEA_DATA
  {
  bool     POS_OK;
  bool     RMC_OK;
  bool     EPOCH_VALID;
  bool     SAT_VALID;
  uint8_t  FIX_Q;
  uint8_t  FIX_TP;
  uint8_t  SAT_USED;
  uint8_t  GPS_VIS;
  uint8_t  GLO_VIS;
  uint8_t  GAL_VIS;
  uint8_t  BDS_VIS;
  uint8_t  GPS_USE;
  uint8_t  GLO_USE;
  uint8_t  GAL_USE;
  uint8_t  BDS_USE;
  float    PDOP;
  float    HDOP;
  float    VDOP;
  double   LAT;
  double   LON;
  double   ALT;
  double   GEOID;
  uint32_t UTC_HMS;
  uint32_t UTC_DATE;
  uint32_t RX_MS;
  uint32_t POS_MS;
  uint32_t EPOCH_MS;
  uint32_t NMEA_OK;
  uint32_t NMEA_BAD_CHECKSUM;
  uint32_t NMEA_OVERFLOW;
  float    GPS_CNO_AVG;
  float    GLO_CNO_AVG;
  float    GAL_CNO_AVG;
  float    BDS_CNO_AVG;
  float    GPS_CNO_MAX;
  float    GLO_CNO_MAX;
  float    GAL_CNO_MAX;
  float    BDS_CNO_MAX;
  char     MODE[8];
  char     DATA[320];
  bool     _VIS[NMEA_SYSN][128];
  bool     _USE[NMEA_SYSN][128];
  uint16_t _CNO_SUM[NMEA_SYSN];
  uint16_t _CNO_CNT[NMEA_SYSN];
  uint8_t  _CNO_MAX[NMEA_SYSN];
  uint8_t  _EPOCH_MASK;
  uint8_t  _GSV_COMPLETE;
  };
//==============================================================================
static inline uint8_t _NMEA_SYS_GSV(const char *TYP)
  {
  if(!strncmp(TYP,"GP",2)) return NMEA_GPS;
  if(!strncmp(TYP,"GL",2)) return NMEA_GLO;
  if(!strncmp(TYP,"GA",2)) return NMEA_GAL;
  if(!strncmp(TYP,"GB",2) || !strncmp(TYP,"BD",2)) return NMEA_BDS;
  return 255;
  }
//------------------------------------------------------------------------------
static inline uint8_t _NMEA_SYS_GSA(const char *TYP,const char *SID)
  {
  if(SID && SID[0])
    {
    uint8_t I=(uint8_t)atoi(SID);
    if(I==1) return NMEA_GPS;
    if(I==2) return NMEA_GLO;
    if(I==3) return NMEA_GAL;
    if(I==4) return NMEA_BDS;
    }
  return _NMEA_SYS_GSV(TYP);
  }
//------------------------------------------------------------------------------
static inline double _NMEA_DEC(const char *VAL,const char *HEM)
  {
  if(!VAL || !VAL[0]) return 0.0;
  double RAW=atof(VAL);
  int DEG=(int)(RAW/100.0);
  double MIN=RAW - ((double)DEG*100.0);
  double OUT=(double)DEG + (MIN/60.0);
  if(HEM && (HEM[0]=='S' || HEM[0]=='W')) OUT=-OUT;
  return OUT;
  }
//------------------------------------------------------------------------------
static inline bool _NMEA_COORD(const char *VAL,const char *HEM,
                               bool LATITUDE,double &OUT)
  {
  if(!VAL || !VAL[0] || !HEM || !HEM[0]) return false;
  char *END=nullptr;
  double RAW=strtod(VAL,&END);
  if(END==VAL || *END!='\0' || !isfinite(RAW) || RAW<0.0) return false;
  int DEG=(int)(RAW/100.0);
  double MIN=RAW-(double)DEG*100.0;
  if(MIN<0.0 || MIN>=60.0) return false;
  if(LATITUDE)
    {
    if((HEM[0]!='N' && HEM[0]!='S') || DEG>90) return false;
    }
  else if((HEM[0]!='E' && HEM[0]!='W') || DEG>180) return false;
  OUT=(double)DEG+MIN/60.0;
  if(HEM[0]=='S' || HEM[0]=='W') OUT=-OUT;
  return LATITUDE ? OUT>=-90.0 && OUT<=90.0 : OUT>=-180.0 && OUT<=180.0;
  }
//------------------------------------------------------------------------------
static inline uint32_t _NMEA_HMS(const char *TXT)
  {
  if(!TXT || !TXT[0]) return 0;
  return (uint32_t)atof(TXT);
  }
//------------------------------------------------------------------------------
static inline uint32_t _NMEA_DATE(const char *TXT)
  {
  if(!TXT || !TXT[0]) return 0;
  return (uint32_t)atoi(TXT);
  }
//------------------------------------------------------------------------------
static inline bool _NMEA_UINT(const char *TXT,int MINV,int MAXV,uint8_t &OUT)
  {
  if(!TXT || !TXT[0]) return false;
  char *END=nullptr;
  long V=strtol(TXT,&END,10);
  if(END==TXT || *END!='\0' || V<MINV || V>MAXV) return false;
  OUT=(uint8_t)V;
  return true;
  }
//------------------------------------------------------------------------------
static inline bool _NMEA_FLOAT(const char *TXT,float MINV,float MAXV,float &OUT)
  {
  if(!TXT || !TXT[0]) return false;
  char *END=nullptr;
  float V=strtof(TXT,&END);
  if(END==TXT || *END!='\0' || !isfinite(V) || V<MINV || V>MAXV)
    return false;
  OUT=V;
  return true;
  }
//------------------------------------------------------------------------------
static inline void _NMEA_CLR_EPOCH(NMEA_DATA &D)
  {
  memset(D._VIS,0,sizeof(D._VIS));
  memset(D._USE,0,sizeof(D._USE));
  memset(D._CNO_SUM,0,sizeof(D._CNO_SUM));
  memset(D._CNO_CNT,0,sizeof(D._CNO_CNT));
  memset(D._CNO_MAX,0,sizeof(D._CNO_MAX));
  D.GPS_VIS=D.GLO_VIS=D.GAL_VIS=D.BDS_VIS=0;
  D.GPS_USE=D.GLO_USE=D.GAL_USE=D.BDS_USE=0;
  D.GPS_CNO_AVG=D.GLO_CNO_AVG=D.GAL_CNO_AVG=D.BDS_CNO_AVG=0;
  D.GPS_CNO_MAX=D.GLO_CNO_MAX=D.GAL_CNO_MAX=D.BDS_CNO_MAX=0;
  D._EPOCH_MASK=0;
  D._GSV_COMPLETE=0;
  D.EPOCH_VALID=false;
  }
//------------------------------------------------------------------------------
static inline void NMEA_RESET(NMEA_DATA &D)
  {
  memset(&D,0,sizeof(D));
  D.MODE[0]='\0';
  D.DATA[0]='\0';
  }
//==============================================================================
// GNSS BINARY PAYLOAD - Platform-independent encoding
//==============================================================================
static inline uint8_t _NMEA_ENC_DOP10(float V)
  {
  if(isnan(V) || V<0) return 0;
  int X=(int)(V*10.0f+0.5f);
  if(X<0) X=0;
  if(X>255) X=255;
  return (uint8_t)X;
  }
//------------------------------------------------------------------------------
static inline uint8_t _NMEA_ENC_DOP7(float V)
  {
  uint8_t X=_NMEA_ENC_DOP10(V);
  return (X>127)?127:X;
  }
//------------------------------------------------------------------------------
static inline int16_t _NMEA_ENC_ALT10(double V)
  {
  long X=lround(V*10.0);
  if(X<-32768L) X=-32768L;
  if(X>32767L)  X=32767L;
  return (int16_t)X;
  }
//------------------------------------------------------------------------------
static inline int32_t _NMEA_ENC_LL1E6(double V)
  {
  return (int32_t)lround(V*1000000.0);
  }
//------------------------------------------------------------------------------
static inline uint8_t _NMEA_ENC_SAT6(uint8_t V)
  {
  return (V>63)?63:V;
  }
//------------------------------------------------------------------------------
static inline void NMEA_PACK17(uint8_t *PAY,const NMEA_DATA &D)
  {
  int32_t LATI=_NMEA_ENC_LL1E6(D.LAT);
  int32_t LONI=_NMEA_ENC_LL1E6(D.LON);
  int16_t ALTI=_NMEA_ENC_ALT10(D.ALT);
  bool SATX=D.GPS_VIS>63 || D.BDS_VIS>63 || D.GLO_VIS>63 ||
            D.GAL_VIS>63 || D.SAT_USED>63;
  uint32_t SATP=((uint32_t)_NMEA_ENC_SAT6(D.GPS_VIS)<<26) |
                ((uint32_t)_NMEA_ENC_SAT6(D.BDS_VIS)<<20) |
                ((uint32_t)_NMEA_ENC_SAT6(D.GLO_VIS)<<14) |
                ((uint32_t)_NMEA_ENC_SAT6(D.GAL_VIS)<<8)  |
                ((uint32_t)_NMEA_ENC_SAT6(D.SAT_USED)<<2) |
                ((uint32_t)D.FIX_TP&0x03);
  // GPSV, BDSV, GLOV, GALV and SATU use 6 bits; FIXT uses 2 bits.
  PAY[0]=(uint8_t)((SATP>>24)&0xFF);
  PAY[1]=(uint8_t)((SATP>>16)&0xFF);
  PAY[2]=(uint8_t)((SATP>>8)&0xFF);
  PAY[3]=(uint8_t)(SATP&0xFF);
  // PDOP uses 7 bits; bit 7 reports a saturated satellite counter.
  PAY[4]=_NMEA_ENC_DOP7(D.PDOP)|(SATX?0x80:0x00);
  PAY[5]=_NMEA_ENC_DOP10(D.HDOP);
  PAY[6]=_NMEA_ENC_DOP10(D.VDOP);
  PAY[7]=(uint8_t)((LATI>>24)&0xFF);
  PAY[8]=(uint8_t)((LATI>>16)&0xFF);
  PAY[9]=(uint8_t)((LATI>>8)&0xFF);
  PAY[10]=(uint8_t)(LATI&0xFF);
  PAY[11]=(uint8_t)((LONI>>24)&0xFF);
  PAY[12]=(uint8_t)((LONI>>16)&0xFF);
  PAY[13]=(uint8_t)((LONI>>8)&0xFF);
  PAY[14]=(uint8_t)(LONI&0xFF);
  PAY[15]=highByte(ALTI);
  PAY[16]=lowByte(ALTI);
  }
//------------------------------------------------------------------------------
static inline void _NMEA_SET_EPOCH(NMEA_DATA &D,uint32_t HMS)
  {
  if(HMS==0) return;
  if(D.UTC_HMS!=0 && D.UTC_HMS!=HMS) _NMEA_CLR_EPOCH(D);
  D.UTC_HMS=HMS;
  }
//------------------------------------------------------------------------------
static inline uint8_t _NMEA_CNT_BOOL(bool *ARR,uint8_t N)
  {
  uint8_t C=0;
  for(uint8_t I=0;I<N;I++) if(ARR[I]) C++;
  return C;
  }
//------------------------------------------------------------------------------
static inline bool _NMEA_SPLIT(char *LINE,char *F[],uint8_t &N)
  {
  N=0;
  char *S=strchr(LINE,'$');
  if(!S) return false;
  char *AST=strchr(S,'*');
  if(!AST || !AST[1] || !AST[2] || AST[3]!='\0') return false;
  uint8_t SUM=0;
  for(char *P=S+1;P<AST;P++) SUM^=(uint8_t)*P;
  char CHECKSUM_TEXT[3]={AST[1],AST[2],'\0'};
  char *END=nullptr;
  long EXPECTED=strtol(CHECKSUM_TEXT,&END,16);
  if(END!=CHECKSUM_TEXT+2 || EXPECTED<0 || EXPECTED>255 ||
     SUM!=(uint8_t)EXPECTED)
    return false;
  *AST='\0';
  char *P=S+1;
  F[N++]=P;
  while(*P && N<32)
    {
    if(*P==',') { *P='\0'; F[N++]=P+1; }
    P++;
    }
  return N>0;
  }
//------------------------------------------------------------------------------
static inline void NMEA_FINISH(NMEA_DATA &D)
  {
  D.GPS_VIS=_NMEA_CNT_BOOL(D._VIS[NMEA_GPS],128);
  D.GLO_VIS=_NMEA_CNT_BOOL(D._VIS[NMEA_GLO],128);
  D.GAL_VIS=_NMEA_CNT_BOOL(D._VIS[NMEA_GAL],128);
  D.BDS_VIS=_NMEA_CNT_BOOL(D._VIS[NMEA_BDS],128);
  D.GPS_USE=_NMEA_CNT_BOOL(D._USE[NMEA_GPS],128);
  D.GLO_USE=_NMEA_CNT_BOOL(D._USE[NMEA_GLO],128);
  D.GAL_USE=_NMEA_CNT_BOOL(D._USE[NMEA_GAL],128);
  D.BDS_USE=_NMEA_CNT_BOOL(D._USE[NMEA_BDS],128);
  if(D._CNO_CNT[NMEA_GPS]) D.GPS_CNO_AVG=(float)D._CNO_SUM[NMEA_GPS]/(float)D._CNO_CNT[NMEA_GPS];
  if(D._CNO_CNT[NMEA_GLO]) D.GLO_CNO_AVG=(float)D._CNO_SUM[NMEA_GLO]/(float)D._CNO_CNT[NMEA_GLO];
  if(D._CNO_CNT[NMEA_GAL]) D.GAL_CNO_AVG=(float)D._CNO_SUM[NMEA_GAL]/(float)D._CNO_CNT[NMEA_GAL];
  if(D._CNO_CNT[NMEA_BDS]) D.BDS_CNO_AVG=(float)D._CNO_SUM[NMEA_BDS]/(float)D._CNO_CNT[NMEA_BDS];
  D.GPS_CNO_MAX=(float)D._CNO_MAX[NMEA_GPS];
  D.GLO_CNO_MAX=(float)D._CNO_MAX[NMEA_GLO];
  D.GAL_CNO_MAX=(float)D._CNO_MAX[NMEA_GAL];
  D.BDS_CNO_MAX=(float)D._CNO_MAX[NMEA_BDS];
  bool POSITION_FRESH=D.POS_MS!=0 &&
                      (uint32_t)(millis()-D.POS_MS)<=NMEA_FIX_MAX_AGE_MS;
  uint16_t VISIBLE=(uint16_t)D.GPS_VIS+(uint16_t)D.GLO_VIS+
                   (uint16_t)D.GAL_VIS+(uint16_t)D.BDS_VIS;
  // A constellation used by GSA is coherent only after its own GSV cycle
  // is complete and contains at least the same number of visible satellites.
  bool GPS_OK=D.GPS_USE==0 ||
              ((D._GSV_COMPLETE&0x01) && D.GPS_USE<=D.GPS_VIS);
  bool GLO_OK=D.GLO_USE==0 ||
              ((D._GSV_COMPLETE&0x02) && D.GLO_USE<=D.GLO_VIS);
  bool GAL_OK=D.GAL_USE==0 ||
              ((D._GSV_COMPLETE&0x04) && D.GAL_USE<=D.GAL_VIS);
  bool BDS_OK=D.BDS_USE==0 ||
              ((D._GSV_COMPLETE&0x08) && D.BDS_USE<=D.BDS_VIS);
  D.SAT_VALID=D._GSV_COMPLETE!=0 && D.SAT_USED<=VISIBLE &&
              GPS_OK && GLO_OK && GAL_OK && BDS_OK;
  D.EPOCH_VALID=D.POS_OK && POSITION_FRESH &&
                (D._EPOCH_MASK&0x01) && (D._EPOCH_MASK&0x02) &&
                D.SAT_VALID && D.FIX_TP>=2 &&
                isfinite(D.LAT) && isfinite(D.LON) &&
                D.LAT>=-90.0 && D.LAT<=90.0 &&
                D.LON>=-180.0 && D.LON<=180.0;
  }
//------------------------------------------------------------------------------
static inline bool NMEA_PARSE_LINE(char *LINE,NMEA_DATA &D)
  {
  char *F[32]; uint8_t N=0;
  if(!_NMEA_SPLIT(LINE,F,N))
    {
    D.NMEA_BAD_CHECKSUM++;
    return false;
    }
  D.NMEA_OK++;
  D.RX_MS=millis();
  char *TYP=F[0];
  size_t TL=strlen(TYP);
  if(TL<5) return false;
  const char *MID=TYP+2;
  if(!strcmp(MID,"RMC"))
    {
    if(N>1) _NMEA_SET_EPOCH(D,_NMEA_HMS(F[1]));
    D.RMC_OK=(N>2 && F[2][0]=='A');
    if(N>9 && F[9][0]) D.UTC_DATE=_NMEA_DATE(F[9]);
    if(N>6 && D.RMC_OK)
      {
      double LAT=0.0,LON=0.0;
      if(_NMEA_COORD(F[3],F[4],true,LAT) &&
         _NMEA_COORD(F[5],F[6],false,LON))
        {
        D.LAT=LAT;
        D.LON=LON;
        D.POS_MS=millis();
        }
      }
    NMEA_FINISH(D);
    return true;
    }
  if(!strcmp(MID,"GGA"))
    {
    if(N>1) _NMEA_SET_EPOCH(D,_NMEA_HMS(F[1]));
    if(N>6) _NMEA_UINT(F[6],0,8,D.FIX_Q);
    if(N>7) _NMEA_UINT(F[7],0,99,D.SAT_USED);
    if(N>8) _NMEA_FLOAT(F[8],0.0f,99.9f,D.HDOP);
    float VALUE=0.0f;
    if(N>11 && _NMEA_FLOAT(F[11],-200.0f,200.0f,VALUE)) D.GEOID=VALUE;
    D.POS_OK=(D.FIX_Q>0);
    if(N>9 && D.POS_OK &&
       _NMEA_FLOAT(F[9],-1000.0f,20000.0f,VALUE)) D.ALT=VALUE;
    if(N>5 && D.POS_OK)
      {
      double LAT=0.0,LON=0.0;
      D.POS_OK=_NMEA_COORD(F[2],F[3],true,LAT) &&
               _NMEA_COORD(F[4],F[5],false,LON);
      if(D.POS_OK)
        {
        D.LAT=LAT;
        D.LON=LON;
        D.POS_MS=millis();
        D.EPOCH_MS=D.POS_MS;
        D._EPOCH_MASK|=0x01;
        }
      }
    NMEA_FINISH(D);
    return true;
    }
  if(!strcmp(MID,"GNS"))
    {
    if(N>1) _NMEA_SET_EPOCH(D,_NMEA_HMS(F[1]));
    if(N>6 && F[6][0]) { strncpy(D.MODE,F[6],sizeof(D.MODE)-1); D.MODE[sizeof(D.MODE)-1]='\0'; }
    if(N>7) _NMEA_UINT(F[7],0,99,D.SAT_USED);
    if(N>8) _NMEA_FLOAT(F[8],0.0f,99.9f,D.HDOP);
    float VALUE=0.0f;
    if(N>10 && _NMEA_FLOAT(F[10],-200.0f,200.0f,VALUE)) D.GEOID=VALUE;
    D.POS_OK=false;
    for(uint8_t I=0;I<strlen(D.MODE);I++) if(D.MODE[I]!='N') { D.POS_OK=true; break; }
    if(N>9 && D.POS_OK &&
       _NMEA_FLOAT(F[9],-1000.0f,20000.0f,VALUE)) D.ALT=VALUE;
    if(N>5 && D.POS_OK)
      {
      double LAT=0.0,LON=0.0;
      D.POS_OK=_NMEA_COORD(F[2],F[3],true,LAT) &&
               _NMEA_COORD(F[4],F[5],false,LON);
      if(D.POS_OK)
        {
        D.LAT=LAT;
        D.LON=LON;
        D.POS_MS=millis();
        D.EPOCH_MS=D.POS_MS;
        D._EPOCH_MASK|=0x01;
        }
      }
    NMEA_FINISH(D);
    return true;
    }
  if(!strcmp(MID,"GSA"))
    {
    if(N>17)
      {
      uint8_t SYS=_NMEA_SYS_GSA(TYP,N>18?F[18]:nullptr);
      if(SYS<NMEA_SYSN)
        {
        if(N>2) _NMEA_UINT(F[2],1,3,D.FIX_TP);
        for(uint8_t I=3;I<=14 && I<N;I++)
          {
          if(F[I][0])
            {
            int SV=atoi(F[I]);
            if(SV>=0 && SV<128) D._USE[SYS][SV]=true;
            }
          }
        }
      if(N>15) _NMEA_FLOAT(F[15],0.0f,99.9f,D.PDOP);
      if(N>16) _NMEA_FLOAT(F[16],0.0f,99.9f,D.HDOP);
      if(N>17) _NMEA_FLOAT(F[17],0.0f,99.9f,D.VDOP);
      D._EPOCH_MASK|=0x02;
      NMEA_FINISH(D);
      return true;
      }
    }
  if(!strcmp(MID,"GSV"))
    {
    uint8_t SYS=_NMEA_SYS_GSV(TYP);
    if(SYS<NMEA_SYSN)
      {
      int TOTAL=N>1?atoi(F[1]):0;
      int NUMBER=N>2?atoi(F[2]):0;
      if(TOTAL<1 || TOTAL>16 || NUMBER<1 || NUMBER>TOTAL) return false;
      if(NUMBER==1)
        {
        memset(D._VIS[SYS],0,sizeof(D._VIS[SYS]));
        D._CNO_SUM[SYS]=0;
        D._CNO_CNT[SYS]=0;
        D._CNO_MAX[SYS]=0;
        D._GSV_COMPLETE&=(uint8_t)~(1U<<SYS);
        }
      for(uint8_t I=4;I+3<N;I+=4)
        {
        int SV=F[I][0] ? atoi(F[I]) : -1;
        int CN=F[I+3][0] ? atoi(F[I+3]) : -1;
        if(SV>=0 && SV<128) D._VIS[SYS][SV]=true;
        if(CN>=0 && CN<=99)
          {
          D._CNO_SUM[SYS]+=(uint16_t)CN;
          D._CNO_CNT[SYS]++;
          if((uint8_t)CN>D._CNO_MAX[SYS]) D._CNO_MAX[SYS]=(uint8_t)CN;
          }
        }
      if(NUMBER==TOTAL) D._GSV_COMPLETE|=(uint8_t)(1U<<SYS);
      NMEA_FINISH(D);
      return true;
      }
    }
  return false;
  }
//------------------------------------------------------------------------------
static inline void NMEA_FEED(Stream &SERIF,char *LINE,uint16_t &LPTR,size_t LMAX,NMEA_DATA &GBD,bool PRNRAW=false)
  {
  while(SERIF.available())
    {
    char C=(char)SERIF.read();
    if(C=='\r') continue;
    if(C=='\n')
      {
      LINE[LPTR]='\0';
      if(LPTR>0)
        {
        if(PRNRAW){ Serial.print("RAW | "); Serial.println(LINE); }
        NMEA_PARSE_LINE(LINE,GBD);
        }
      LPTR=0;
      continue;
      }
    if(C=='$' && LPTR>0) LPTR=0;
    if(LPTR<(LMAX-1)) LINE[LPTR++]=C;
    else
      {
      LPTR=0;
      GBD.NMEA_OVERFLOW++;
      }
    }
  }
//------------------------------------------------------------------------------
static inline void NMEA_PARSE_COPY(const char *RAW,NMEA_DATA &GBD)
  {
  const char *NMEA=RAW?strchr(RAW,'$'):nullptr;
  if(!NMEA) return;
  size_t N=strnlen(NMEA,255);
  while(N && (NMEA[N-1]=='\r' || NMEA[N-1]=='\n')) N--;
  if(!N || N>=255) { GBD.NMEA_OVERFLOW++; return; }
  char LINE[256];
  memcpy(LINE,NMEA,N);
  LINE[N]='\0';
  NMEA_PARSE_LINE(LINE,GBD);
  }
//------------------------------------------------------------------------------
static inline void NMEA_BUILD_DATA(NMEA_DATA &D)
  {
  NMEA_FINISH(D);
  bool COK=(D.LAT!=0.0 || D.LON!=0.0);
  snprintf(D.DATA,sizeof(D.DATA),
           "NMEA.DT | T:%06lu,D:%06lu,POS:%d,EPC:%d,SOK:%d,AGE:%lu,RMC:%d,MODE:%s,COK:%d,FQ:%u,FT:%u,SU:%u,LAT:%.9f,LON:%.9f,ALT:%.2f,PDOP:%.2f,HDOP:%.2f,VDOP:%.2f,GPSV:%u,GLOV:%u,GALV:%u,BDSV:%u,GPSU:%u,GLOU:%u,GALU:%u,BDSU:%u,GPSC:%.0f/%.0f,GLOC:%.0f/%.0f,GALC:%.0f/%.0f,BDSC:%.0f/%.0f,NOK:%lu,NCS:%lu,NOV:%lu,GSV:%02X",
           (unsigned long)D.UTC_HMS,
           (unsigned long)D.UTC_DATE,
           D.POS_OK,
           D.EPOCH_VALID,
           D.SAT_VALID,
           D.POS_MS?(unsigned long)(millis()-D.POS_MS):0UL,
           D.RMC_OK,
           D.MODE[0]?D.MODE:"------",
           COK,
           (unsigned)D.FIX_Q,
           (unsigned)D.FIX_TP,
           (unsigned)D.SAT_USED,
           D.LAT,D.LON,D.ALT,
           (double)D.PDOP,(double)D.HDOP,(double)D.VDOP,
           (unsigned)D.GPS_VIS,(unsigned)D.GLO_VIS,(unsigned)D.GAL_VIS,(unsigned)D.BDS_VIS,
           (unsigned)D.GPS_USE,(unsigned)D.GLO_USE,(unsigned)D.GAL_USE,(unsigned)D.BDS_USE,
           (double)D.GPS_CNO_AVG,(double)D.GPS_CNO_MAX,
           (double)D.GLO_CNO_AVG,(double)D.GLO_CNO_MAX,
           (double)D.GAL_CNO_AVG,(double)D.GAL_CNO_MAX,
           (double)D.BDS_CNO_AVG,(double)D.BDS_CNO_MAX,
           (unsigned long)D.NMEA_OK,
           (unsigned long)D.NMEA_BAD_CHECKSUM,
           (unsigned long)D.NMEA_OVERFLOW,
           (unsigned)D._GSV_COMPLETE);
  }
//==============================================================================
#endif
//==============================================================================
