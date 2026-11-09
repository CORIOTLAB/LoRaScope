//==============================================================================
// CORIOT_M10.h
//==============================================================================
// Configuration and query driver for u-blox M10 GNSS family
// Protocol : UBX binary (CFG-* + CFG-VALSET + CFG-VALGET) + UBX-MON polling
// Tested   : u-blox UBX-M10050-KB ROM SPG 5.10 (7b202e)
//==============================================================================
#ifndef CORIOT_M10_H
#define CORIOT_M10_H
//==============================================================================
#include <Arduino.h>
#include <stdlib.h>
#include <CORIOT_ATOMS3_PLATFORM.h>
//==============================================================================
#ifndef M10_ACK_TIMEOUT_MS
  #define M10_ACK_TIMEOUT_MS  500UL
#endif
#ifndef M10_GNSS_RESTART_MS
  #define M10_GNSS_RESTART_MS 250UL
#endif
//==============================================================================
// Structs
//==============================================================================
struct M10_INRS
  {
  bool OK;
  char CFG[96];
  };
//------------------------------------------------------------------------------
struct M10_VER
  {
  bool OK;
  char SW[30];
  char HW[10];
  char DATA[96];
  };
inline M10_VER M10_POLL_VER(CORIOT_SERIAL_TYPE &SRIF);
//------------------------------------------------------------------------------
struct M10_GNSS_STATUS
  {
  bool    OK;
  bool    GPS;
  bool    GLO;
  bool    BDS;
  bool    GAL;
  uint8_t SUP;
  uint8_t DEF;
  uint8_t ENA;
  uint8_t SIM;
  char    DATA[128];
  };
//------------------------------------------------------------------------------
struct M10_NAV5
  {
  bool    OK;
  uint8_t DYNMODEL;
  uint8_t FIXMODE;
  int8_t  ELEVMASK;
  uint8_t MINSV;
  char    DATA[96];
  };
//------------------------------------------------------------------------------
struct M10_NMEA_CFG
  {
  uint8_t GGA;
  uint8_t RMC;
  uint8_t GSA;
  uint8_t GSV;
  uint8_t GNS;
  uint8_t GLL;
  uint8_t VTG;
  uint8_t ZDA;
  };
//------------------------------------------------------------------------------
struct M10_CFG_RES
  {
  bool OK;
  bool RATE;
  bool NAV5;
  bool GNSS;
  bool SAVE;
  char DATA[128];
  };
//------------------------------------------------------------------------------
struct M10_OPT_RES
  {
  bool OK;
  bool QZSS;
  bool SBAS;
  bool NMEA;
  char DATA[128];
  };
//------------------------------------------------------------------------------
struct M10_VERIFY_RES
  {
  bool OK;
  M10_VER VER;
  M10_NAV5 NAV;
  M10_GNSS_STATUS GNS;
  char L1[128];
  char L2[96];
  char L3[96];
  char DATA[128];
  };
//------------------------------------------------------------------------------
struct M10_GNSS_KEYS
  {
  bool OK;
  uint8_t GPS_ENA;
  uint8_t GPS_L1CA;
  uint8_t GAL_ENA;
  uint8_t GAL_E1;
  uint8_t BDS_ENA;
  uint8_t BDS_B1C;
  uint8_t GLO_ENA;
  uint8_t GLO_L1;
  uint8_t QZSS_ENA;
  uint8_t QZSS_L1CA;
  uint8_t SBAS_ENA;
  uint8_t SBAS_L1CA;
  char DATA[160];
  };
//------------------------------------------------------------------------------
struct M10_QZSS_STATUS
  {
  bool OK;
  uint8_t ENA;
  uint8_t L1CA;
  uint8_t L1S;
  char DATA[96];
  };
//------------------------------------------------------------------------------
struct M10_SBAS_STATUS
  {
  bool OK;
  uint8_t SIG_ENA;
  uint8_t L1CA;
  uint8_t USE_TESTMODE;
  uint8_t USE_RANGING;
  uint8_t USE_DIFFCORR;
  uint8_t USE_INTEGRITY;
  char DATA[128];
  };
//==============================================================================
//  UBX helpers
//==============================================================================
inline void _M10_SEND(CORIOT_SERIAL_TYPE &SRIF,uint8_t CLS,uint8_t ID,const uint8_t *PAY,uint16_t PLEN)
  {
  uint8_t HDR[6]={0xB5,0x62,CLS,ID,(uint8_t)(PLEN&0xFF),(uint8_t)(PLEN>>8)};
  SRIF.write(HDR,6);
  if(PLEN>0 && PAY!=nullptr) SRIF.write(PAY,PLEN);
  uint8_t CKA=0,CKB=0;
  CKA+=CLS; CKB+=CKA;
  CKA+=ID;  CKB+=CKA;
  CKA+=(uint8_t)(PLEN&0xFF); CKB+=CKA;
  CKA+=(uint8_t)(PLEN>>8);   CKB+=CKA;
  for(uint16_t I=0;I<PLEN && PAY!=nullptr;I++) { CKA+=PAY[I]; CKB+=CKA; }
  SRIF.write(CKA);
  SRIF.write(CKB);
  SRIF.flush();
  }
//------------------------------------------------------------------------------
inline int8_t _M10_WAIT_ACK(CORIOT_SERIAL_TYPE &SRIF,uint8_t CLS,uint8_t ID)
  {
  uint8_t ST=0,ACLS=0,AID=0,ATYP=0;
  uint32_t T0=millis();
  while((millis()-T0)<M10_ACK_TIMEOUT_MS)
    {
    if(!SRIF.available()) { yield(); continue; }
    uint8_t B=(uint8_t)SRIF.read();
    switch(ST)
      {
      case 0: if(B==0xB5) ST=1; break;
      case 1: ST=(B==0x62)?2:0; break;
      case 2: ST=(B==0x05)?3:(B==0xB5?1:0); break;
      case 3:
        if(B==0x01){ATYP=1;ST=4;}
        else if(B==0x00){ATYP=0;ST=4;}
        else ST=(B==0xB5)?1:0;
        break;
      case 4: ST=(B==0x02)?5:0; break;
      case 5: ST=(B==0x00)?6:0; break;
      case 6: ACLS=B; ST=7; break;
      case 7: AID=B; if(ACLS==CLS && AID==ID) return (int8_t)ATYP; ST=0; break;
      }
    }
  return -1;
  }
//------------------------------------------------------------------------------
inline bool _M10_RECV_PAYLOAD(CORIOT_SERIAL_TYPE &SRIF,uint8_t CLS,uint8_t ID,uint8_t *PBUF,uint16_t BUFLEN,uint16_t &PLEN,uint32_t TIMEOUT_MS=M10_ACK_TIMEOUT_MS)
  {
  uint8_t ST=0;
  uint16_t PCNT=0;
  PLEN=0;
  uint32_t T0=millis();
  while((millis()-T0)<TIMEOUT_MS)
    {
    if(!SRIF.available()) { yield(); continue; }
    uint8_t B=(uint8_t)SRIF.read();
    switch(ST)
      {
      case 0: if(B==0xB5) ST=1; break;
      case 1: ST=(B==0x62)?2:0; break;
      case 2: ST=(B==CLS)?3:(B==0xB5?1:0); break;
      case 3: ST=(B==ID)?4:(B==0xB5?1:0); break;
      case 4: PLEN=(uint16_t)B; ST=5; break;
      case 5: PLEN|=((uint16_t)B<<8); PCNT=0; ST=6; break;
      case 6:
        if(PCNT<BUFLEN) PBUF[PCNT]=B;
        PCNT++;
        if(PCNT>=PLEN) return true;
        break;
      }
    }
  return false;
  }
//------------------------------------------------------------------------------
inline int8_t _M10_VALSET_U1(CORIOT_SERIAL_TYPE &SRIF,const uint32_t *KEYS,const uint8_t *VALS,uint8_t N)
  {
  uint8_t PLEN=(uint8_t)(4+N*5);
  uint8_t PAY[4+8*5];
  PAY[0]=0x00; PAY[1]=0x07; PAY[2]=0x00; PAY[3]=0x00;
  for(uint8_t I=0;I<N;I++)
    {
    uint8_t *P=PAY+4+I*5;
    P[0]=(uint8_t)(KEYS[I]&0xFF);
    P[1]=(uint8_t)((KEYS[I]>>8)&0xFF);
    P[2]=(uint8_t)((KEYS[I]>>16)&0xFF);
    P[3]=(uint8_t)((KEYS[I]>>24)&0xFF);
    P[4]=VALS[I];
    }
  _M10_SEND(SRIF,0x06,0x8A,PAY,PLEN);
  return _M10_WAIT_ACK(SRIF,0x06,0x8A);
  }
//------------------------------------------------------------------------------
inline bool _M10_VALGET_U1(CORIOT_SERIAL_TYPE &SRIF,const uint32_t *KEYS,uint8_t *VALS,uint8_t N)
  {
  uint8_t PAY[4+8*4];
  uint8_t PLEN=(uint8_t)(4+N*4);
  PAY[0]=0x00; PAY[1]=0x00; PAY[2]=0x00; PAY[3]=0x00; // version=0 layer=RAM pos=0
  for(uint8_t I=0;I<N;I++)
    {
    uint8_t *P=PAY+4+I*4;
    P[0]=(uint8_t)(KEYS[I]&0xFF);
    P[1]=(uint8_t)((KEYS[I]>>8)&0xFF);
    P[2]=(uint8_t)((KEYS[I]>>16)&0xFF);
    P[3]=(uint8_t)((KEYS[I]>>24)&0xFF);
    VALS[I]=0;
    }
  _M10_SEND(SRIF,0x06,0x8B,PAY,PLEN);
  uint8_t PBUF[128];
  uint16_t RLEN=0;
  if(!_M10_RECV_PAYLOAD(SRIF,0x06,0x8B,PBUF,sizeof(PBUF),RLEN)) return false;
  if(RLEN<4) return false;
  for(uint8_t I=0;I<N;I++)
    {
    bool FND=false;
    for(uint16_t P=4;P+4<RLEN;P=(uint16_t)(P+5))
      {
      uint32_t K=(uint32_t)PBUF[P] | ((uint32_t)PBUF[P+1]<<8) | ((uint32_t)PBUF[P+2]<<16) | ((uint32_t)PBUF[P+3]<<24);
      if(K==KEYS[I]) { VALS[I]=PBUF[P+4]; FND=true; break; }
      }
    if(!FND) return false;
    }
  return true;
  }
//==============================================================================
//  Config messages
//==============================================================================
inline bool M10_SET_RATE(CORIOT_SERIAL_TYPE &SRIF,uint16_t MEASRATE_MS,uint16_t NAVRATE=1,uint16_t TIMEREF=1)
  {
  uint8_t PAY[6];
  PAY[0]=(uint8_t)(MEASRATE_MS&0xFF); PAY[1]=(uint8_t)(MEASRATE_MS>>8);
  PAY[2]=(uint8_t)(NAVRATE&0xFF);     PAY[3]=(uint8_t)(NAVRATE>>8);
  PAY[4]=(uint8_t)(TIMEREF&0xFF);     PAY[5]=(uint8_t)(TIMEREF>>8);
  _M10_SEND(SRIF,0x06,0x08,PAY,6);
  return (_M10_WAIT_ACK(SRIF,0x06,0x08)==1);
  }
//------------------------------------------------------------------------------
inline bool M10_SET_BAUD(CORIOT_SERIAL_TYPE &SRIF,uint32_t BAUD)
  {
  uint8_t PAY[20]={0};
  PAY[0]=0x01;
  PAY[4]=0xD0; PAY[5]=0x08;
  PAY[8]=(uint8_t)(BAUD&0xFF);
  PAY[9]=(uint8_t)((BAUD>>8)&0xFF);
  PAY[10]=(uint8_t)((BAUD>>16)&0xFF);
  PAY[11]=(uint8_t)((BAUD>>24)&0xFF);
  PAY[12]=0x03; PAY[14]=0x03;
  _M10_SEND(SRIF,0x06,0x00,PAY,20);
  bool OK=(_M10_WAIT_ACK(SRIF,0x06,0x00)==1);
  delay(100);
  SRIF.begin(BAUD);
  return OK;
  }
//------------------------------------------------------------------------------
#define M10_NMEA_CLS  0xF0
#define M10_MSG_GGA   0x00
#define M10_MSG_GLL   0x01
#define M10_MSG_GSA   0x02
#define M10_MSG_GSV   0x03
#define M10_MSG_RMC   0x04
#define M10_MSG_VTG   0x05
#define M10_MSG_GNS   0x0D
#define M10_MSG_ZDA   0x08
//------------------------------------------------------------------------------
inline bool M10_SET_MSG(CORIOT_SERIAL_TYPE &SRIF,uint8_t MSGCLS,uint8_t MSGID,uint8_t RATE)
  {
  uint8_t PAY[8]={MSGCLS,MSGID,0,RATE,0,0,0,0};
  _M10_SEND(SRIF,0x06,0x01,PAY,8);
  return (_M10_WAIT_ACK(SRIF,0x06,0x01)==1);
  }
//==============================================================================
//  GNSS signal keys
//==============================================================================
#define M10_KEY_GPS_ENA        0x1031001FUL
#define M10_KEY_GPS_L1CA_ENA   0x10310001UL
#define M10_KEY_SBAS_ENA       0x10310020UL
#define M10_KEY_SBAS_L1CA_ENA  0x10310005UL
#define M10_KEY_GAL_ENA        0x10310021UL
#define M10_KEY_GAL_E1_ENA     0x10310007UL
#define M10_KEY_BDS_ENA        0x10310022UL
#define M10_KEY_BDS_B1_ENA     0x1031000DUL
#define M10_KEY_BDS_B1C_ENA    0x1031000FUL
#define M10_KEY_QZSS_ENA       0x10310024UL
#define M10_KEY_QZSS_L1CA_ENA  0x10310012UL
#define M10_KEY_QZSS_L1S_ENA   0x10310014UL
#define M10_KEY_GLO_ENA        0x10310025UL
#define M10_KEY_GLO_L1_ENA     0x10310018UL
//------------------------------------------------------------------------------
// NOTE — UBX-M10050-KB ROM SPG 5.10 on this unit:
//   MON-GNSS reports SUP:0x0F (GPS+GLO+BDS+GAL supported), but field tests show
//   unstable / inconsistent ACK behavior when forcing GLO and some BDS signal keys.
//   In u-center GLONASS can be enabled, but on this unit it does not reliably end
//   up tracking/using that constellation in practice. Keep GLO experimental here.
//------------------------------------------------------------------------------
inline bool M10_SET_GNSS_SEL(CORIOT_SERIAL_TYPE &SRIF,bool ENA_GPS,bool ENA_GAL,bool ENA_BDS,bool ENA_GLO)
  {
  struct { uint32_t ENA; uint32_t SIG; uint8_t VAL; } CLIST[4]=
    {
    { M10_KEY_GPS_ENA, M10_KEY_GPS_L1CA_ENA, (uint8_t)(ENA_GPS?1:0) },
    { M10_KEY_GAL_ENA, M10_KEY_GAL_E1_ENA,   (uint8_t)(ENA_GAL?1:0) },
    { M10_KEY_BDS_ENA, M10_KEY_BDS_B1C_ENA,  (uint8_t)(ENA_BDS?1:0) },
    { M10_KEY_GLO_ENA, M10_KEY_GLO_L1_ENA,   (uint8_t)(ENA_GLO?1:0) },
    };
  bool OK=true;
  for(uint8_t I=0;I<4;I++)
    {
    uint32_t KEYS[2]={CLIST[I].ENA,CLIST[I].SIG};
    uint8_t  VALS[2]={CLIST[I].VAL,CLIST[I].VAL};
    if(_M10_VALSET_U1(SRIF,KEYS,VALS,2)!=1) OK=false;
    delay(M10_GNSS_RESTART_MS);
    }
  return OK;
  }
//------------------------------------------------------------------------------
inline bool M10_SET_GNSS_ALL(CORIOT_SERIAL_TYPE &SRIF)
  {
  return M10_SET_GNSS_SEL(SRIF,true,true,true,true);
  }
//==============================================================================
//  NAV settings
//==============================================================================
#define M10_DYN_PORTABLE    0
#define M10_DYN_STATIONARY  2
#define M10_DYN_PEDESTRIAN  3
#define M10_DYN_AUTOMOTIVE  4
#define M10_DYN_SEA         5
#define M10_DYN_AIR1G       6
#define M10_DYN_AIR2G       7
#define M10_DYN_AIR4G       8
//------------------------------------------------------------------------------
inline bool M10_SET_NAV5(CORIOT_SERIAL_TYPE &SRIF,uint8_t MODEL,uint8_t MINSV=5,int8_t ELEVMASK=10)
  {
  static const uint32_t KEYS[3]={0x20110021UL,0x201100A4UL,0x201100A1UL};
  uint8_t VALS[3]={MODEL,(uint8_t)ELEVMASK,MINSV};
  return (_M10_VALSET_U1(SRIF,KEYS,VALS,3)==1);
  }
//------------------------------------------------------------------------------
inline bool M10_SET_DYNMODEL(CORIOT_SERIAL_TYPE &SRIF,uint8_t MODEL)
  {
  static const uint32_t KEY=0x20110021UL;
  return (_M10_VALSET_U1(SRIF,&KEY,&MODEL,1)==1);
  }
//==============================================================================
//  SBAS / QZSS setters
//==============================================================================
inline bool M10_SET_SBAS(CORIOT_SERIAL_TYPE &SRIF,bool ENA=true)
  {
  uint8_t PAY[8]={0};
  PAY[0]=ENA?0x01:0x00;
  PAY[1]=ENA?0x07:0x00;
  PAY[2]=ENA?0x03:0x00;
  _M10_SEND(SRIF,0x06,0x16,PAY,8);
  return (_M10_WAIT_ACK(SRIF,0x06,0x16)==1);
  }
//------------------------------------------------------------------------------
inline bool M10_SET_QZSS(CORIOT_SERIAL_TYPE &SRIF,bool ENA=true)
  {
  uint32_t KEYS[2]={M10_KEY_QZSS_ENA,M10_KEY_QZSS_L1CA_ENA};
  uint8_t  VALS[2]={(uint8_t)(ENA?1:0),(uint8_t)(ENA?1:0)};
  return (_M10_VALSET_U1(SRIF,KEYS,VALS,2)==1);
  }
//==============================================================================
//  Reset / init
//==============================================================================
#define M10_NMEA_V411  0x23
#define M10_NMEA_V410  0x21
#define M10_NMEA_V23   0x17
//------------------------------------------------------------------------------
inline bool M10_RESET(CORIOT_SERIAL_TYPE &SRIF,uint16_t BBRMASK,uint8_t MODE)
  {
  uint8_t PAY[4]={(uint8_t)(BBRMASK&0xFF),(uint8_t)(BBRMASK>>8),MODE,0x00};
  _M10_SEND(SRIF,0x06,0x04,PAY,4);
  delay(500);
  while(SRIF.available()) SRIF.read();
  return true;
  }
inline bool M10_COLD_RESET(CORIOT_SERIAL_TYPE &SRIF){return M10_RESET(SRIF,0xFFFF,0x01);} 
inline bool M10_WARM_RESET(CORIOT_SERIAL_TYPE &SRIF){return M10_RESET(SRIF,0x0001,0x01);} 
inline bool M10_HOT_RESET (CORIOT_SERIAL_TYPE &SRIF){return M10_RESET(SRIF,0x0000,0x01);} 
//------------------------------------------------------------------------------
inline M10_INRS M10_INIT(CORIOT_SERIAL_TYPE &SRIF,uint32_t BAUD=115200)
  {
  M10_INRS I; I.OK=false; I.CFG[0]='\0';
  (void)BAUD;
  delay(100);
  while(SRIF.available()) SRIF.read();
  M10_VER V=M10_POLL_VER(SRIF);
  snprintf(I.CFG,sizeof(I.CFG),"M10.I | Baud:%lu VER:%s",
           (unsigned long)BAUD,V.OK?V.SW:"TIMEOUT");
  I.OK=V.OK;
  return I;
  }
//==============================================================================
//  Polls
//==============================================================================
inline M10_VER M10_POLL_VER(CORIOT_SERIAL_TYPE &SRIF)
  {
  M10_VER V; V.OK=false; V.SW[0]='\0'; V.HW[0]='\0'; V.DATA[0]='\0';
  uint8_t PBUF[80];
  uint16_t PLEN=0;
  _M10_SEND(SRIF,0x0A,0x04,nullptr,0);
  if(_M10_RECV_PAYLOAD(SRIF,0x0A,0x04,PBUF,sizeof(PBUF),PLEN) && PLEN>=40)
    {
    uint8_t SI=0; for(;SI<29 && SI<PLEN && PBUF[SI];SI++) V.SW[SI]=PBUF[SI]; V.SW[SI]='\0';
    uint8_t HI=0; for(;HI<9 && (30+HI)<PLEN && PBUF[30+HI];HI++) V.HW[HI]=PBUF[30+HI]; V.HW[HI]='\0';
    snprintf(V.DATA,sizeof(V.DATA),"M10.V | SW:%s | HW:%s",V.SW,V.HW);
    V.OK=true;
    }
  else snprintf(V.DATA,sizeof(V.DATA),"M10.V | TIMEOUT");
  return V;
  }
//------------------------------------------------------------------------------
inline M10_GNSS_STATUS M10_POLL_GNSS(CORIOT_SERIAL_TYPE &SRIF)
  {
  M10_GNSS_STATUS S;
  S.OK=false; S.GPS=S.GLO=S.BDS=S.GAL=false; S.SUP=0; S.DEF=0; S.ENA=0; S.SIM=0; S.DATA[0]='\0';
  uint8_t PBUF[12];
  uint16_t PLEN=0;
  _M10_SEND(SRIF,0x0A,0x28,nullptr,0);
  if(_M10_RECV_PAYLOAD(SRIF,0x0A,0x28,PBUF,sizeof(PBUF),PLEN) && PLEN>=5)
    {
    S.SUP=PBUF[1]; S.DEF=PBUF[2]; S.ENA=PBUF[3]; S.SIM=PBUF[4];
    S.GPS=(S.ENA&0x01)!=0; S.GLO=(S.ENA&0x02)!=0; S.BDS=(S.ENA&0x04)!=0; S.GAL=(S.ENA&0x08)!=0;
    snprintf(S.DATA,sizeof(S.DATA),"M10.G | SUP:0x%02X DEF:0x%02X ENA:0x%02X SIM:%u | GPS:%d GLO:%d BDS:%d GAL:%d",S.SUP,S.DEF,S.ENA,(unsigned)S.SIM,S.GPS,S.GLO,S.BDS,S.GAL);
    S.OK=true;
    }
  else snprintf(S.DATA,sizeof(S.DATA),"M10.G | TIMEOUT");
  return S;
  }
//------------------------------------------------------------------------------
inline M10_NAV5 M10_POLL_NAV5(CORIOT_SERIAL_TYPE &SRIF)
  {
  M10_NAV5 N; N.OK=false; N.DYNMODEL=0; N.FIXMODE=0; N.ELEVMASK=0; N.MINSV=0; N.DATA[0]='\0';
  static const uint32_t KEYS[3]={0x20110021UL,0x201100A4UL,0x201100A1UL};
  uint8_t VALS[3];
  if(_M10_VALGET_U1(SRIF,KEYS,VALS,3))
    {
    N.DYNMODEL=VALS[0]; N.ELEVMASK=(int8_t)VALS[1]; N.MINSV=VALS[2]; N.OK=true;
    snprintf(N.DATA,sizeof(N.DATA),"M10.N | DYN:%d ELEV:%ddeg MINSV:%d",N.DYNMODEL,N.ELEVMASK,N.MINSV);
    }
  else snprintf(N.DATA,sizeof(N.DATA),"M10.N | TIMEOUT");
  return N;
  }
//------------------------------------------------------------------------------
inline M10_GNSS_KEYS M10_POLL_GNSS_KEYS(CORIOT_SERIAL_TYPE &SRIF)
  {
  M10_GNSS_KEYS G;
  G.OK=false;
  G.GPS_ENA=G.GPS_L1CA=G.GAL_ENA=G.GAL_E1=G.BDS_ENA=G.BDS_B1C=G.GLO_ENA=G.GLO_L1=0;
  G.QZSS_ENA=G.QZSS_L1CA=G.SBAS_ENA=G.SBAS_L1CA=0;
  G.DATA[0]='\0';
  static const uint32_t KEYS[12]={M10_KEY_GPS_ENA,M10_KEY_GPS_L1CA_ENA,M10_KEY_GAL_ENA,M10_KEY_GAL_E1_ENA,M10_KEY_BDS_ENA,M10_KEY_BDS_B1C_ENA,M10_KEY_GLO_ENA,M10_KEY_GLO_L1_ENA,M10_KEY_QZSS_ENA,M10_KEY_QZSS_L1CA_ENA,M10_KEY_SBAS_ENA,M10_KEY_SBAS_L1CA_ENA};
  uint8_t V[12];
  if(_M10_VALGET_U1(SRIF,KEYS,V,12))
    {
    G.GPS_ENA=V[0]; G.GPS_L1CA=V[1]; G.GAL_ENA=V[2]; G.GAL_E1=V[3]; G.BDS_ENA=V[4]; G.BDS_B1C=V[5];
    G.GLO_ENA=V[6]; G.GLO_L1=V[7]; G.QZSS_ENA=V[8]; G.QZSS_L1CA=V[9]; G.SBAS_ENA=V[10]; G.SBAS_L1CA=V[11];
    snprintf(G.DATA,sizeof(G.DATA),"M10.K | GPS:%u/%u GAL:%u/%u BDS:%u/%u GLO:%u/%u QZSS:%u/%u SBAS:%u/%u",G.GPS_ENA,G.GPS_L1CA,G.GAL_ENA,G.GAL_E1,G.BDS_ENA,G.BDS_B1C,G.GLO_ENA,G.GLO_L1,G.QZSS_ENA,G.QZSS_L1CA,G.SBAS_ENA,G.SBAS_L1CA);
    G.OK=true;
    }
  else snprintf(G.DATA,sizeof(G.DATA),"M10.K | TIMEOUT");
  return G;
  }
//------------------------------------------------------------------------------
inline M10_QZSS_STATUS M10_POLL_QZSS(CORIOT_SERIAL_TYPE &SRIF)
  {
  M10_QZSS_STATUS Q; Q.OK=false; Q.ENA=Q.L1CA=Q.L1S=0; Q.DATA[0]='\0';
  static const uint32_t KEYS[3]={M10_KEY_QZSS_ENA,M10_KEY_QZSS_L1CA_ENA,M10_KEY_QZSS_L1S_ENA};
  uint8_t V[3];
  if(_M10_VALGET_U1(SRIF,KEYS,V,3))
    {
    Q.ENA=V[0]; Q.L1CA=V[1]; Q.L1S=V[2]; Q.OK=true;
    snprintf(Q.DATA,sizeof(Q.DATA),"M10.Q | ENA:%u L1CA:%u L1S:%u",Q.ENA,Q.L1CA,Q.L1S);
    }
  else snprintf(Q.DATA,sizeof(Q.DATA),"M10.Q | TIMEOUT");
  return Q;
  }
//------------------------------------------------------------------------------
inline M10_SBAS_STATUS M10_POLL_SBAS(CORIOT_SERIAL_TYPE &SRIF)
  {
  M10_SBAS_STATUS S; S.OK=false; S.SIG_ENA=S.L1CA=S.USE_TESTMODE=S.USE_RANGING=S.USE_DIFFCORR=S.USE_INTEGRITY=0; S.DATA[0]='\0';
  static const uint32_t KEYS[6]={M10_KEY_SBAS_ENA,M10_KEY_SBAS_L1CA_ENA,0x10360002UL,0x10360003UL,0x10360004UL,0x10360005UL};
  uint8_t V[6];
  if(_M10_VALGET_U1(SRIF,KEYS,V,6))
    {
    S.SIG_ENA=V[0]; S.L1CA=V[1]; S.USE_TESTMODE=V[2]; S.USE_RANGING=V[3]; S.USE_DIFFCORR=V[4]; S.USE_INTEGRITY=V[5]; S.OK=true;
    snprintf(S.DATA,sizeof(S.DATA),"M10.SB| SIG:%u L1CA:%u TEST:%u RNG:%u DIFF:%u INT:%u",S.SIG_ENA,S.L1CA,S.USE_TESTMODE,S.USE_RANGING,S.USE_DIFFCORR,S.USE_INTEGRITY);
    }
  else snprintf(S.DATA,sizeof(S.DATA),"M10.SB| TIMEOUT");
  return S;
  }
//==============================================================================
//  Effective validation helpers
//==============================================================================
inline bool M10_GNSS_CFG_EFFECTIVE(const M10_GNSS_STATUS &MON,const M10_GNSS_KEYS &KEYS,
                                   bool ENA_GPS,bool ENA_GAL,bool ENA_BDS,bool ENA_GLO)
  {
  bool OK=true;
  if(ENA_GPS){ if(!(MON.GPS && KEYS.GPS_ENA && KEYS.GPS_L1CA)) OK=false; }
  else       { if( MON.GPS || KEYS.GPS_ENA || KEYS.GPS_L1CA) OK=false; }
  if(ENA_GAL){ if(!(MON.GAL && KEYS.GAL_ENA && KEYS.GAL_E1)) OK=false; }
  else       { if( MON.GAL || KEYS.GAL_ENA || KEYS.GAL_E1) OK=false; }
  // BDS on this unit is considered effective if the constellation is active and BDS_ENA=1.
  // BDS_B1C_ENA may read back 0 while MON-GNSS still reports BDS active and usable.
  if(ENA_BDS){ if(!(MON.BDS && KEYS.BDS_ENA)) OK=false; }
  else       { if( MON.BDS || KEYS.BDS_ENA) OK=false; }
  // GLONASS remains experimental on this unit: require both monitor + keys when enabled.
  if(ENA_GLO){ if(!(MON.GLO && KEYS.GLO_ENA && KEYS.GLO_L1)) OK=false; }
  else       { if( MON.GLO || KEYS.GLO_ENA || KEYS.GLO_L1) OK=false; }
  return OK;
  }
//------------------------------------------------------------------------------
inline bool M10_QZSS_CFG_EFFECTIVE(const M10_QZSS_STATUS &Q,bool ENA_QZSS)
  {
  // Treat L1S as informational; the effective off state is ENA=0 and L1CA=0.
  if(ENA_QZSS) return (Q.ENA && Q.L1CA);
  return (!Q.ENA && !Q.L1CA);
  }
//------------------------------------------------------------------------------
inline bool M10_SBAS_CFG_EFFECTIVE(const M10_SBAS_STATUS &S,bool ENA_SBAS)
  {
  if(ENA_SBAS) return (S.SIG_ENA && S.L1CA);
  return (!S.SIG_ENA && !S.L1CA);
  }
//==============================================================================
//  Save / helpers
//==============================================================================
inline bool M10_SAVE_CFG(CORIOT_SERIAL_TYPE &SRIF)
  {
  uint8_t PAY[12]={0x00,0x00,0x00,0x00,0x1F,0x06,0x00,0x00,0x00,0x00,0x00,0x00};
  _M10_SEND(SRIF,0x06,0x09,PAY,12);
  return (_M10_WAIT_ACK(SRIF,0x06,0x09)==1);
  }
//------------------------------------------------------------------------------
inline bool M10_CFG_NMEA(CORIOT_SERIAL_TYPE &SRIF,const M10_NMEA_CFG &N)
  {
  bool OK=true;
  OK=M10_SET_MSG(SRIF,M10_NMEA_CLS,M10_MSG_GGA,N.GGA)&&OK;
  OK=M10_SET_MSG(SRIF,M10_NMEA_CLS,M10_MSG_RMC,N.RMC)&&OK;
  OK=M10_SET_MSG(SRIF,M10_NMEA_CLS,M10_MSG_GSA,N.GSA)&&OK;
  OK=M10_SET_MSG(SRIF,M10_NMEA_CLS,M10_MSG_GSV,N.GSV)&&OK;
  OK=M10_SET_MSG(SRIF,M10_NMEA_CLS,M10_MSG_GNS,N.GNS)&&OK;
  OK=M10_SET_MSG(SRIF,M10_NMEA_CLS,M10_MSG_GLL,N.GLL)&&OK;
  OK=M10_SET_MSG(SRIF,M10_NMEA_CLS,M10_MSG_VTG,N.VTG)&&OK;
  OK=M10_SET_MSG(SRIF,M10_NMEA_CLS,M10_MSG_ZDA,N.ZDA)&&OK;
  return OK;
  }
//------------------------------------------------------------------------------
inline M10_CFG_RES M10_CFG_BASIC(CORIOT_SERIAL_TYPE &SRIF,uint16_t RATE_MS,uint8_t DYNMODEL,uint8_t MINSV,int8_t ELEVMASK,bool ENA_GPS,bool ENA_GAL,bool ENA_BDS,bool ENA_GLO,bool SAVE=true)
  {
  M10_CFG_RES R; R.OK=false;
  R.RATE=M10_SET_RATE(SRIF,RATE_MS);
  R.NAV5=M10_SET_NAV5(SRIF,DYNMODEL,MINSV,ELEVMASK);
  (void)M10_SET_GNSS_SEL(SRIF,ENA_GPS,ENA_GAL,ENA_BDS,ENA_GLO);
  M10_GNSS_STATUS MON=M10_POLL_GNSS(SRIF);
  M10_GNSS_KEYS   KEY=M10_POLL_GNSS_KEYS(SRIF);
  R.GNSS=M10_GNSS_CFG_EFFECTIVE(MON,KEY,ENA_GPS,ENA_GAL,ENA_BDS,ENA_GLO);
  R.SAVE=SAVE?M10_SAVE_CFG(SRIF):true;
  R.OK=R.RATE&&R.NAV5&&R.GNSS&&R.SAVE;
  snprintf(R.DATA,sizeof(R.DATA),"RATE:%d NAV5:%d GNSS:%d SAVE:%d",R.RATE,R.NAV5,R.GNSS,R.SAVE);
  return R;
  }
//------------------------------------------------------------------------------
inline M10_OPT_RES M10_CFG_OPT(CORIOT_SERIAL_TYPE &SRIF,bool ENA_QZSS,bool ENA_SBAS,const M10_NMEA_CFG &N)
  {
  M10_OPT_RES R; R.OK=false;
  (void)M10_SET_QZSS(SRIF,ENA_QZSS);
  (void)M10_SET_SBAS(SRIF,ENA_SBAS);
  M10_QZSS_STATUS Q=M10_POLL_QZSS(SRIF);
  M10_SBAS_STATUS S=M10_POLL_SBAS(SRIF);
  R.QZSS=M10_QZSS_CFG_EFFECTIVE(Q,ENA_QZSS);
  R.SBAS=M10_SBAS_CFG_EFFECTIVE(S,ENA_SBAS);
  R.NMEA=M10_CFG_NMEA(SRIF,N);
  R.OK=R.QZSS&&R.SBAS&&R.NMEA;
  snprintf(R.DATA,sizeof(R.DATA),"QZSS:%d SBAS:%d NMEA:%d",R.QZSS,R.SBAS,R.NMEA);
  return R;
  }
//------------------------------------------------------------------------------
#ifndef M10_START_NONE
  #define M10_START_NONE  0
#endif
#ifndef M10_START_HOT
  #define M10_START_HOT   1
#endif
#ifndef M10_START_WARM
  #define M10_START_WARM  2
#endif
#ifndef M10_START_COLD
  #define M10_START_COLD  3
#endif
//------------------------------------------------------------------------------
inline const char *M10_START_STR(uint8_t MODE)
  {
  switch(MODE)
    {
    case M10_START_HOT:  return "HOT";
    case M10_START_WARM: return "WARM";
    case M10_START_COLD: return "COLD";
    default:             return "NONE";
    }
  }
//------------------------------------------------------------------------------
inline bool M10_APPLY_START(CORIOT_SERIAL_TYPE &SRIF,uint8_t MODE,uint32_t BAUD=115200,uint32_t WAIT_MS=1500)
  {
  if(MODE==M10_START_NONE) return true;
  if(MODE==M10_START_HOT) M10_HOT_RESET(SRIF);
  else if(MODE==M10_START_WARM) M10_WARM_RESET(SRIF);
  else if(MODE==M10_START_COLD) M10_COLD_RESET(SRIF);
  else return false;
  delay(WAIT_MS);
  SRIF.begin(BAUD);
  delay(100);
  while(SRIF.available()) SRIF.read();
  return true;
  }
//------------------------------------------------------------------------------
inline bool M10_FACTORY_RESET(CORIOT_SERIAL_TYPE &SRIF,uint32_t BAUD=115200,uint32_t WAIT_MS=1500)
  {
  M10_COLD_RESET(SRIF);
  delay(WAIT_MS);
  SRIF.begin(BAUD);
  delay(100);
  while(SRIF.available()) SRIF.read();
  return true;
  }
//------------------------------------------------------------------------------
inline M10_VERIFY_RES M10_VERIFY(CORIOT_SERIAL_TYPE &SRIF)
  {
  M10_VERIFY_RES R; R.OK=false;
  R.VER=M10_POLL_VER(SRIF);
  R.NAV=M10_POLL_NAV5(SRIF);
  R.GNS=M10_POLL_GNSS(SRIF);
  R.OK=R.VER.OK&&R.NAV.OK&&R.GNS.OK;
  snprintf(R.L1,sizeof(R.L1),"M10.I | FW:%s | CFG:%s",R.VER.OK?R.VER.SW:"TIMEOUT",R.OK?"OK":"CHECK");
  snprintf(R.L2,sizeof(R.L2),"M10.N | DYN:%u ELEV:%d MINSV:%u",(unsigned)R.NAV.DYNMODEL,(int)R.NAV.ELEVMASK,(unsigned)R.NAV.MINSV);
  snprintf(R.L3,sizeof(R.L3),"M10.G | ENA:0x%02X | GPS:%d GLO:%d BDS:%d GAL:%d",R.GNS.ENA,R.GNS.GPS,R.GNS.GLO,R.GNS.BDS,R.GNS.GAL);
  snprintf(R.DATA,sizeof(R.DATA),"VER:%d NAV:%d GNS:%d",R.VER.OK,R.NAV.OK,R.GNS.OK);
  return R;
  }
//==============================================================================
#endif
//==============================================================================
