//==============================================================================
// CORIOT_A152_RADIO.h
//==============================================================================
#ifndef CORIOT_A152_RADIO_H
#define CORIOT_A152_RADIO_H
//------------------------------------------------------------------------------
#include <Arduino.h>
#include <stdarg.h>
#include "rak3172_lorawan.hpp"
#include <CORIOT_ATOMS3_PLATFORM.h>
#include <CORIOT_general.h>
//==============================================================================
struct A152_RADIO_CONFIG
  {
  const char *DEVEUI;
  const char *APPEUI;
  const char *APPKEY;
  const char *CHANNEL_MASK;
  bool ADR;
  uint8_t DATA_RATE;
  bool CONFIRMED;
  };
//------------------------------------------------------------------------------
struct A152_DEV
  {
  RAK3172LoRaWAN MOD;
  bool INIT;
  bool JOIN_REQ;
  bool JOIN_OK;
  bool TASK_OK;
  bool ACK_PENDING;
  bool BUSY;
  bool CONFIRMED;
  uint32_t ACK_T0;
  uint32_t UP_CO;
  uint32_t ACK_CO;
  uint32_t RX_CO;
  uint32_t NO_ACK_CO;
  uint8_t TX_POWER;
  uint8_t FLG;
  const char *PTH;
  CORIOT_MUTEX_HANDLE MUTEX;
  };
//==============================================================================
static A152_DEV *A152_CTX=nullptr;
//==============================================================================
static inline void A152_RESET(A152_DEV &DEV,uint8_t FLG,
                              const char *PTH=nullptr)
  {
  DEV.INIT=false;
  DEV.JOIN_REQ=false;
  DEV.JOIN_OK=false;
  DEV.TASK_OK=false;
  DEV.ACK_PENDING=false;
  DEV.BUSY=false;
  DEV.CONFIRMED=false;
  DEV.ACK_T0=0;
  DEV.UP_CO=0;
  DEV.ACK_CO=0;
  DEV.RX_CO=0;
  DEV.NO_ACK_CO=0;
  DEV.TX_POWER=255;
  DEV.FLG=FLG;
  DEV.PTH=PTH;
  DEV.MUTEX=CORIOT_MUTEX_CREATE();
  }
//==============================================================================
static inline void A152_LOGF(const char *FMT,...)
  {
  if(!A152_CTX) return;
  char OUT[384];
  va_list AP;
  va_start(AP,FMT);
  vsnprintf(OUT,sizeof(OUT),FMT,AP);
  va_end(AP);
  LOG_WRITE(A152_CTX->FLG,A152_CTX->PTH,OUT);
  }
//------------------------------------------------------------------------------
static inline bool A152_LOG_RES(A152_DEV &DEV,const char *CODE,bool OK)
  {
  char OUT[32];
  snprintf(OUT,sizeof(OUT),"A152.%s | %s\r\n",CODE,OK?"OK":"ER");
  LOG_WRITE(DEV.FLG,DEV.PTH,OUT);
  return OK;
  }
//==============================================================================
static void A152_JOIN_CB(bool STATUS)
  {
  if(!A152_CTX) return;
  A152_CTX->JOIN_OK=STATUS;
  A152_LOG_RES(*A152_CTX,"JN",STATUS);
  if(STATUS)
    {
    String BAND=A152_CTX->MOD.getBAND(); BAND.trim();
    String MASK=A152_CTX->MOD.getChannelMask(); MASK.trim();
    String ADDR=A152_CTX->MOD.getDevAddr(); ADDR.trim();
    String NETI=A152_CTX->MOD.getNetworkId(); NETI.trim();
    String DRAT=A152_CTX->MOD.getCommand("AT+DR=?"); DRAT.trim();
    String RX1D=A152_CTX->MOD.getRX1Delay(); RX1D.trim();
    A152_LOGF("A152.ST | BAND:%s MASK:%s ADDR:%s NETID:%s DR:%s RX1DL:%s\r\n",
              BAND.c_str(),MASK.c_str(),ADDR.c_str(),NETI.c_str(),DRAT.c_str(),RX1D.c_str());
    }
  }
//------------------------------------------------------------------------------
static void A152_RECV_CB(lorawan_frame_t FRAME)
  {
  if(!A152_CTX) return;
  A152_CTX->RX_CO++;
  A152_LOGF("A152.RX | PORT:%d RSSI:%d SNR:%d LEN:%d\r\n",
            FRAME.port,FRAME.rssi,FRAME.snr,FRAME.len);
  }
//------------------------------------------------------------------------------
static void A152_ERR_CB(char *ERR)
  {
  if(!A152_CTX) return;
  A152_LOGF("A152.ER | %s\r\n",ERR?ERR:"UNKNOWN");
  }
//==============================================================================
static inline void A152_UPDATE(A152_DEV &DEV)
  {
  if(!DEV.MUTEX || !CORIOT_MUTEX_LOCK(DEV.MUTEX,25UL)) return;
  DEV.MOD.update();
  if(DEV.MOD.available())
    {
    std::vector<lorawan_frame_t> FRAMES=DEV.MOD.read();
    for(size_t I=0;I<FRAMES.size();I++) A152_RECV_CB(FRAMES[I]);
    DEV.MOD.flush();
    }
  if(DEV.ACK_PENDING && (uint32_t)(millis()-DEV.ACK_T0)>=6000UL)
    {
    String CFS=DEV.MOD.getCommand("AT+CFS=?");
    CFS.trim();
    DEV.ACK_PENDING=false;
    if(CFS=="1")
      {
      DEV.ACK_CO++;
      A152_LOGF("A152.RX | ACK UC:%lu AC:%lu RX:%lu\r\n",
                (unsigned long)DEV.UP_CO,(unsigned long)DEV.ACK_CO,
                (unsigned long)DEV.RX_CO);
      }
    else
      {
      DEV.NO_ACK_CO++;
      A152_LOGF("A152.RX | NO_ACK CFS:%s NC:%lu\r\n",CFS.c_str(),
                (unsigned long)DEV.NO_ACK_CO);
      }
    }
  CORIOT_MUTEX_UNLOCK(DEV.MUTEX);
  }
//------------------------------------------------------------------------------
static void A152_TASK(void *ARG)
  {
  A152_DEV *DEV=(A152_DEV*)ARG;
  for(;;)
    {
    A152_UPDATE(*DEV);
    CORIOT_TASK_WAIT(5);
    }
  }
//==============================================================================
static inline bool A152_INIT(A152_DEV &DEV,CORIOT_SERIAL_TYPE *SERIF,
                             int8_t RXPIN,int8_t TXPIN,
                             const A152_RADIO_CONFIG &CFG)
  {
  A152_CTX=&DEV;
  DEV.INIT=false;
  DEV.CONFIRMED=CFG.CONFIRMED;
  delay(1000);
  for(uint8_t I=0;I<3 && !DEV.INIT;I++)
    {
    while(SERIF->available()) SERIF->read();
    DEV.INIT=DEV.MOD.init(SERIF,RXPIN,TXPIN,RAK3172_BPS_115200);
    if(!DEV.INIT) delay(1200);
    }
  A152_LOG_RES(DEV,"IN",DEV.INIT);
  if(!DEV.INIT) return false;
  bool CFG_OK=true;
  bool OK=DEV.MOD.setBAND(AU915,CFG.CHANNEL_MASK);
  A152_LOG_RES(DEV,"CF.BAND",OK); CFG_OK=OK&&CFG_OK;
  delay(300);
  OK=DEV.MOD.sendCommand("AT+RX1DL=1");
  A152_LOG_RES(DEV,"CF.RX1D",OK); CFG_OK=OK&&CFG_OK;
  delay(100);
  OK=DEV.MOD.sendCommand("AT+JN1DL=5");
  A152_LOG_RES(DEV,"CF.JN1D",OK); CFG_OK=OK&&CFG_OK;
  delay(100);
  OK=DEV.MOD.setOTAA(CFG.DEVEUI,CFG.APPEUI,CFG.APPKEY);
  A152_LOG_RES(DEV,"CF.OTAA",OK); CFG_OK=OK&&CFG_OK;
  OK=DEV.MOD.setMode(CLASS_A);
  A152_LOG_RES(DEV,"CF.CLASS",OK); CFG_OK=OK&&CFG_OK;
  OK=DEV.MOD.setADR(CFG.ADR);
  A152_LOG_RES(DEV,"CF.ADR",OK); CFG_OK=OK&&CFG_OK;
  OK=DEV.MOD.setDR(CFG.DATA_RATE);
  A152_LOG_RES(DEV,"CF.DR",OK); CFG_OK=OK&&CFG_OK;
  String ADRS=DEV.MOD.getADR(); ADRS.trim();
  String DRS=DEV.MOD.getCommand("AT+DR=?"); DRS.trim();
  bool ADR_READ_OK=(ADRS==(CFG.ADR?"1":"0"));
  bool DR_READ_OK=(DRS==String(CFG.DATA_RATE));
  A152_LOGF("A152.CF.RD | ADR:%s DR:%s REQ_ADR:%u REQ_DR:%u %s\r\n",
            ADRS.c_str(),DRS.c_str(),CFG.ADR?1U:0U,
            (unsigned)CFG.DATA_RATE,
            (ADR_READ_OK&&DR_READ_OK)?"OK":"ER");
  CFG_OK=ADR_READ_OK&&DR_READ_OK&&CFG_OK;
  OK=DEV.MOD.setComfirm(CFG.CONFIRMED);
  A152_LOG_RES(DEV,"CF.CFM",OK); CFG_OK=OK&&CFG_OK;
  OK=DEV.MOD.setLinkCheck(ALLWAYS_LINKCHECK);
  A152_LOG_RES(DEV,"CF.LCHK",OK); CFG_OK=OK&&CFG_OK;
  if(!CFG_OK)
    {
    A152_LOG_RES(DEV,"CF",false);
    return false;
    }
  DEV.MOD.onJoin(A152_JOIN_CB);
  DEV.MOD.onReceive(A152_RECV_CB);
  DEV.MOD.onError(A152_ERR_CB);
  DEV.JOIN_REQ=DEV.MOD.join(true,false,10,8);
  A152_LOG_RES(DEV,"JR",DEV.JOIN_REQ);
  if(!DEV.JOIN_REQ) return false;
  DEV.TASK_OK=CORIOT_TASK_START(A152_TASK,"A152_TASK",1024*6,&DEV,5);
  A152_LOG_RES(DEV,"TK",DEV.TASK_OK);
  return DEV.JOIN_REQ;
  }
//==============================================================================
static inline bool A152_SEND_BIN(A152_DEV &DEV,const uint8_t *PAY,
                                 size_t LEN,uint8_t PORT)
  {
  if(!DEV.MUTEX || !CORIOT_MUTEX_LOCK(DEV.MUTEX,1000UL))
    {
    LOG_WRITE(DEV.FLG,DEV.PTH,"A152.UP | ER BUSY\r\n");
    return false;
    }
  if(!DEV.JOIN_OK)
    {
    CORIOT_MUTEX_UNLOCK(DEV.MUTEX);
    LOG_WRITE(DEV.FLG,DEV.PTH,"A152.UP | ER WAIT_JOIN\r\n");
    return false;
    }
  if(DEV.ACK_PENDING)
    {
    CORIOT_MUTEX_UNLOCK(DEV.MUTEX);
    LOG_WRITE(DEV.FLG,DEV.PTH,"A152.UP | ER ACK_PENDING\r\n");
    return false;
    }
  bool OK=DEV.MOD.send((const uint8_t*)PAY,LEN,PORT);
  A152_LOG_RES(DEV,"UP",OK);
  if(OK)
    {
    DEV.UP_CO++;
    if(DEV.CONFIRMED)
      {
      DEV.ACK_PENDING=true;
      DEV.ACK_T0=millis();
      }
    }
  CORIOT_MUTEX_UNLOCK(DEV.MUTEX);
  return OK;
  }
//==============================================================================
static inline bool A152_VERIFY_TX_POWER(A152_DEV &DEV,uint8_t TX_POWER)
  {
  if(!DEV.MUTEX || !CORIOT_MUTEX_LOCK(DEV.MUTEX,1000UL))
    {
    LOG_WRITE(DEV.FLG,DEV.PTH,"A152.TP | ER BUSY\r\n");
    return false;
    }
  if(!DEV.JOIN_OK)
    {
    CORIOT_MUTEX_UNLOCK(DEV.MUTEX);
    LOG_WRITE(DEV.FLG,DEV.PTH,"A152.UP | ER WAIT_JOIN\r\n");
    return false;
    }
  if(DEV.ACK_PENDING)
    {
    CORIOT_MUTEX_UNLOCK(DEV.MUTEX);
    LOG_WRITE(DEV.FLG,DEV.PTH,"A152.UP | ER ACK_PENDING\r\n");
    return false;
    }
  if(!DEV.MOD.setOutPower(TX_POWER))
    {
    CORIOT_MUTEX_UNLOCK(DEV.MUTEX);
    LOG_WRITE(DEV.FLG,DEV.PTH,"A152.TP | ER SET\r\n");
    return false;
    }
  String TXPS=DEV.MOD.getOutPower();
  TXPS.trim();
  int TXPI=TXPS.toInt();
  if(TXPS.length()==0 || TXPI<0 || TXPI>14 || TXPI!=(int)TX_POWER)
    {
    CORIOT_MUTEX_UNLOCK(DEV.MUTEX);
    A152_LOGF("A152.TP | ER GET:%s\r\n",TXPS.c_str());
    return false;
    }
  DEV.TX_POWER=(uint8_t)TXPI;
  CORIOT_MUTEX_UNLOCK(DEV.MUTEX);
  return true;
  }
//==============================================================================
static inline void A152_GET_COUNTERS(A152_DEV &DEV,uint32_t &UP,uint32_t &ACK)
  {
  if(DEV.MUTEX && CORIOT_MUTEX_LOCK(DEV.MUTEX,25UL))
    {
    UP=DEV.UP_CO;
    ACK=DEV.ACK_CO;
    CORIOT_MUTEX_UNLOCK(DEV.MUTEX);
    }
  }
//==============================================================================
#endif
//==============================================================================
