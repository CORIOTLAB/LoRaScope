//==============================================================================
// CORIOT_ATOMS3_DISPLAY.h
//==============================================================================
#ifndef CORIOT_ATOMS3_DISPLAY_H
#define CORIOT_ATOMS3_DISPLAY_H
//------------------------------------------------------------------------------
#include <M5Unified.h>
#include <CORIOT_NMEA_PARSER.h>
//==============================================================================
static inline void ATOMS3_DRAW_GNSS_STATUS(const NMEA_DATA &GBD,
                                           uint32_t UP_CO,uint32_t ACK_CO)
  {
  uint16_t TVIS=(uint16_t)GBD.GPS_VIS+(uint16_t)GBD.GLO_VIS+
                (uint16_t)GBD.GAL_VIS+(uint16_t)GBD.BDS_VIS;
  M5.Display.clear();
  M5.Display.setRotation(0);
  M5.Display.setTextSize(2);
  M5.Display.setTextColor(GREEN,BLACK);
  M5.Display.setCursor(28,2);
  M5.Display.println("CoRIOT");
  M5.Display.setTextColor(WHITE,BLACK);
  M5.Display.setCursor(0,25);
  M5.Display.print("TV:");
  M5.Display.println(TVIS);
  M5.Display.print("US:");
  M5.Display.println((unsigned)GBD.SAT_USED);
  M5.Display.print("FX:");
  if(GBD.FIX_TP==3)      M5.Display.println("3D");
  else if(GBD.FIX_TP==2) M5.Display.println("2D");
  else                   M5.Display.println("--");
  M5.Display.print("PD:");
  M5.Display.println(GBD.PDOP,1);
  M5.Display.print("UC:");
  M5.Display.println((unsigned long)UP_CO);
  M5.Display.print("AC:");
  M5.Display.println((unsigned long)ACK_CO);
  }
//==============================================================================
#endif
//==============================================================================
