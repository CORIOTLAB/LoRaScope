//==============================================================================
// CORIOT_ADXL345.h
//==============================================================================
// M5 Unit Accel / ADXL345 I2C driver
// Pitch payload: -90..90 degrees, 1-degree steps
// Roll payload : -180..180 degrees, 2-degree steps
//==============================================================================
#ifndef CORIOT_ADXL345_H
#define CORIOT_ADXL345_H
//==============================================================================
#include <Arduino.h>
#include <Wire.h>
#include <math.h>
//==============================================================================
#ifndef ADXL_ADDR
  #define ADXL_ADDR 0x53
#endif
#ifndef ADXL_SDA
  #if defined(BOARD_ADXL_SDA)
    #define ADXL_SDA BOARD_ADXL_SDA
  #else
    #define ADXL_SDA -1
  #endif
#endif
#ifndef ADXL_SCL
  #if defined(BOARD_ADXL_SCL)
    #define ADXL_SCL BOARD_ADXL_SCL
  #else
    #define ADXL_SCL -1
  #endif
#endif
#ifndef ADXL_I2C_HZ
  #if defined(BOARD_ADXL_I2C)
    #define ADXL_I2C_HZ BOARD_ADXL_I2C
  #else
    #define ADXL_I2C_HZ 400000UL
  #endif
#endif
//==============================================================================
#define ADXL_DEVID_REG       0x00
#define ADXL_BW_RATE_REG     0x2C
#define ADXL_POWER_CTL_REG   0x2D
#define ADXL_DATA_FORMAT_REG 0x31
#define ADXL_DATAX0_REG      0x32
#define ADXL_DEVID_VALUE     0xE5
#define ADXL_SAMPLE_MS       10UL
#define ADXL_MAX_AGE_MS      250UL
#ifndef ADXL_OFFSET_X
  #define ADXL_OFFSET_X 0.0f
  #define ADXL_OFFSET_Y 0.0f
  #define ADXL_OFFSET_Z 0.0f
#endif
//==============================================================================
struct ADXL_DATA
  {
  bool INIT;
  bool OK;
  uint8_t ID;
  float X;
  float Y;
  float Z;
  float PITCH;
  float ROLL;
  int8_t PITCH8;
  int8_t ROLL8;
  uint32_t SAMPLE_MS;
  uint32_t SAMPLE_CO;
  uint32_t ERROR_CO;
  TwoWire *WIR;
  };
//==============================================================================
static inline bool _ADXL_WRITE(ADXL_DATA &D,uint8_t REG,uint8_t VAL)
  {
  D.WIR->beginTransmission(ADXL_ADDR);
  D.WIR->write(REG);
  D.WIR->write(VAL);
  return D.WIR->endTransmission()==0;
  }
//------------------------------------------------------------------------------
static inline bool _ADXL_READ(ADXL_DATA &D,uint8_t REG,uint8_t *BUF,size_t LEN)
  {
  D.WIR->beginTransmission(ADXL_ADDR);
  D.WIR->write(REG);
  if(D.WIR->endTransmission(false)!=0) return false;
  size_t N=D.WIR->requestFrom((uint8_t)ADXL_ADDR,(uint8_t)LEN);
  if(N!=LEN) return false;
  for(size_t I=0;I<LEN;I++) BUF[I]=(uint8_t)D.WIR->read();
  return true;
  }
//------------------------------------------------------------------------------
static inline int8_t _ADXL_PACK_PITCH(float V)
  {
  if(isnan(V)) return 0;
  if(V<-90.0f) V=-90.0f;
  if(V> 90.0f) V= 90.0f;
  int X=(int)lroundf(V);
  return (int8_t)X;
  }
//------------------------------------------------------------------------------
static inline int8_t _ADXL_PACK_ROLL(float V)
  {
  if(isnan(V)) return 0;
  if(V<-180.0f) V=-180.0f;
  if(V> 180.0f) V= 180.0f;
  int X=(int)lroundf(V/2.0f);
  return (int8_t)X;
  }
//==============================================================================
static inline bool ADXL_INIT(ADXL_DATA &D,TwoWire &WIR=Wire,
                             int8_t SDA=ADXL_SDA,int8_t SCL=ADXL_SCL)
  {
  memset(&D,0,sizeof(D));
  D.WIR=&WIR;
  D.WIR->begin(SDA,SCL,ADXL_I2C_HZ);
  uint8_t ID=0;
  D.INIT=_ADXL_READ(D,ADXL_DEVID_REG,&ID,1) && ID==ADXL_DEVID_VALUE;
  D.ID=ID;
  if(!D.INIT) return false;
  bool OK=true;
  OK=_ADXL_WRITE(D,ADXL_BW_RATE_REG,0x0A)&&OK;
  OK=_ADXL_WRITE(D,ADXL_DATA_FORMAT_REG,0x08)&&OK;
  OK=_ADXL_WRITE(D,ADXL_POWER_CTL_REG,0x08)&&OK;
  D.INIT=OK;
  return OK;
  }
//==============================================================================
static inline bool ADXL_UPDATE(ADXL_DATA &D)
  {
  if(!D.INIT) return false;
  uint32_t NOW=millis();
  if(D.SAMPLE_MS && (uint32_t)(NOW-D.SAMPLE_MS)<ADXL_SAMPLE_MS) return D.OK;
  uint8_t B[6];
  if(!_ADXL_READ(D,ADXL_DATAX0_REG,B,sizeof(B)))
    {
    D.OK=false;
    D.ERROR_CO++;
    return false;
    }
  int16_t RX=(int16_t)((uint16_t)B[0]|((uint16_t)B[1]<<8));
  int16_t RY=(int16_t)((uint16_t)B[2]|((uint16_t)B[3]<<8));
  int16_t RZ=(int16_t)((uint16_t)B[4]|((uint16_t)B[5]<<8));
  D.X=(float)RX*0.0039f-ADXL_OFFSET_X;
  D.Y=(float)RY*0.0039f-ADXL_OFFSET_Y;
  D.Z=(float)RZ*0.0039f-ADXL_OFFSET_Z;
  float MAG=sqrtf(D.X*D.X+D.Y*D.Y+D.Z*D.Z);
  if(!isfinite(MAG) || MAG<0.5f || MAG>1.5f)
    {
    D.OK=false;
    D.ERROR_CO++;
    D.SAMPLE_MS=NOW;
    return false;
    }
  D.PITCH=atan2f(-D.X,sqrtf(D.Y*D.Y+D.Z*D.Z))*RAD_TO_DEG;
  D.ROLL=atan2f(D.Y,D.Z)*RAD_TO_DEG;
  D.PITCH8=_ADXL_PACK_PITCH(D.PITCH);
  D.ROLL8=_ADXL_PACK_ROLL(D.ROLL);
  D.OK=true;
  D.SAMPLE_MS=NOW;
  D.SAMPLE_CO++;
  return true;
  }
//------------------------------------------------------------------------------
static inline bool ADXL_FRESH(const ADXL_DATA &D)
  {
  return D.OK && D.SAMPLE_MS &&
         (uint32_t)(millis()-D.SAMPLE_MS)<=ADXL_MAX_AGE_MS;
  }
//==============================================================================
#endif
//==============================================================================
