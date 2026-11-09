//==============================================================================
// CORIOT_NMEA_SERVICE.h
//==============================================================================
#ifndef CORIOT_NMEA_SERVICE_H
#define CORIOT_NMEA_SERVICE_H
//==============================================================================
#include <CORIOT_ATOMS3_PLATFORM.h>
#include <CORIOT_NMEA_PARSER.h>
//==============================================================================
typedef void (*NMEA_UPDATE_FN)(NMEA_DATA &D);
//==============================================================================
struct CORIOT_NMEA_SERVICE
  {
  CORIOT_SERIAL_TYPE *SERIF;
  char *LINE;
  uint16_t *LPTR;
  size_t LMAX;
  NMEA_DATA *DATA;
  NMEA_DATA LAST_VALID;
  NMEA_UPDATE_FN UPDATE_FN;
  uint32_t LAST_VALID_NMEA;
  uint32_t SNAP_FALLBACK_CO;
  bool HAS_LAST_VALID;
  bool PRN_RAW;
  bool TASK_OK;
  CORIOT_MUTEX_HANDLE MUTEX;
  };
//------------------------------------------------------------------------------
static inline bool NMEA_SERVICE_UPDATE(CORIOT_NMEA_SERVICE &S)
  {
  if(!S.SERIF || !S.LINE || !S.LPTR || !S.DATA || !S.MUTEX) return false;
  if(!CORIOT_MUTEX_LOCK(S.MUTEX,100UL)) return false;
  if(S.UPDATE_FN) S.UPDATE_FN(*S.DATA);
  else NMEA_FEED(*S.SERIF,S.LINE,*S.LPTR,S.LMAX,*S.DATA,S.PRN_RAW);
  NMEA_FINISH(*S.DATA);
  if(S.DATA->EPOCH_VALID && S.DATA->NMEA_OK!=S.LAST_VALID_NMEA)
    {
    S.LAST_VALID=*S.DATA;
    S.LAST_VALID_NMEA=S.DATA->NMEA_OK;
    S.HAS_LAST_VALID=true;
    }
  CORIOT_MUTEX_UNLOCK(S.MUTEX);
  return true;
  }
//------------------------------------------------------------------------------
static void NMEA_SERVICE_TASK(void *ARG)
  {
  CORIOT_NMEA_SERVICE *S=(CORIOT_NMEA_SERVICE*)ARG;
  for(;;)
    {
    NMEA_SERVICE_UPDATE(*S);
    CORIOT_TASK_WAIT(1);
    }
  }
//------------------------------------------------------------------------------
static inline bool NMEA_SERVICE_START(CORIOT_NMEA_SERVICE &S,
                                      CORIOT_SERIAL_TYPE &SERIF,
                                      char *LINE,uint16_t &LPTR,size_t LMAX,
                                      NMEA_DATA &D,bool PRNRAW=false,
                                      NMEA_UPDATE_FN UPDATE_FN=nullptr)
  {
  S.SERIF=&SERIF;
  S.LINE=LINE;
  S.LPTR=&LPTR;
  S.LMAX=LMAX;
  S.DATA=&D;
  S.UPDATE_FN=UPDATE_FN;
  NMEA_RESET(S.LAST_VALID);
  S.LAST_VALID_NMEA=0;
  S.SNAP_FALLBACK_CO=0;
  S.HAS_LAST_VALID=false;
  S.PRN_RAW=PRNRAW;
  S.TASK_OK=false;
  S.MUTEX=CORIOT_MUTEX_CREATE();
  if(!S.MUTEX) return false;
  S.TASK_OK=CORIOT_TASK_START(NMEA_SERVICE_TASK,"NMEA_TASK",4096,&S,6);
  return S.TASK_OK;
  }
//------------------------------------------------------------------------------
static inline bool NMEA_SERVICE_SNAPSHOT(CORIOT_NMEA_SERVICE &S,
                                         NMEA_DATA &OUT)
  {
  if(!S.DATA || !S.MUTEX) return false;
  if(!CORIOT_MUTEX_LOCK(S.MUTEX,1000UL)) return false;
  OUT=*S.DATA;
  NMEA_FINISH(OUT);
  if(!OUT.EPOCH_VALID && S.HAS_LAST_VALID)
    {
    NMEA_DATA LAST=S.LAST_VALID;
    NMEA_FINISH(LAST);
    if(LAST.EPOCH_VALID)
      {
      OUT=LAST;
      S.SNAP_FALLBACK_CO++;
      }
    }
  CORIOT_MUTEX_UNLOCK(S.MUTEX);
  return true;
  }
//==============================================================================
#endif
//==============================================================================
