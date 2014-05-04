/**********************************************************************************************
 Copyright (c) 2014 DisplayModule. All rights reserved.

 Redistribution and use of this source code, part of this source code or any compiled binary
 based on this source code is permitted as long as the above copyright notice and following
 disclaimer is retained. 

 DISCLAIMER:
 THIS SOFTWARE IS SUPPLIED "AS IS" WITHOUT ANY WARRANTIES AND SUPPORT. DISPLAYMODULE ASSUMES
 NO RESPONSIBILITY OR LIABILITY FOR THE USE OF THE SOFTWARE.
 ********************************************************************************************/

#include "DmTftS6D0164.h"

DmTftS6D0164::DmTftS6D0164(uint8_t wr, uint8_t cs, uint8_t dc, uint8_t rst) : DmTftBase(176, 220) {
  _wr = wr;
  _cs = cs;
  _dc = dc;
  _rst = rst;
}

DmTftS6D0164::~DmTftS6D0164() {
#if defined (TOOLCHAIN_ARM_MICRO)
  delete _pinRST;
  delete _pinCS;
  delete _pinWR;
  delete _pinDC;
  delete _virtualPortD;
  _pinRST = NULL;
  _pinCS = NULL;
  _pinWR = NULL;
  _pinDC = NULL;
  _virtualPortD = NULL;
#endif
}

void DmTftS6D0164::writeBus(uint8_t data) {
#if defined (__AVR__)
  PORTD = data;
#elif defined (TOOLCHAIN_ARM_MICRO)
  *_virtualPortD = data;
#endif
  pulse_low(_pinWR, _bitmaskWR);
}

void DmTftS6D0164::sendCommand(uint8_t index) {
  cbi(_pinDC, _bitmaskDC);
  writeBus(index);
}

void DmTftS6D0164::sendData(uint16_t data) {
  sbi(_pinDC, _bitmaskDC);
  writeBus(data>>8);
  writeBus(data);
}

void DmTftS6D0164::setAddress(uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2) {
  sendCommand(0x37); // Set Column
  sendData(x1);
  sendCommand(0x36); 
  sendData(x2);


  sendCommand(0x39);  // Set Page
  sendData(y1);
  sendCommand(0x38); 
  sendData(y2);
  
  sendCommand(0x20);
  sendData(x1);
  sendCommand(0x21);
  sendData(y1);
  sendCommand(0x22);
}

void DmTftS6D0164::init(void) {
  BACK_COLOR = BLACK;
  POINT_COLOR = WHITE;
#if defined (__AVR__)	
/**************************************
      DM-DmTftS6D016422-102     Arduino UNO             NUM
      
      RST                       A2                      16
      CS                        A3                      17
      WR                        A4                      18
      RS                        A5                      19
      
      DB8                       0                       0
      DB9                       1                       1
      DB10                      2                       2
      DB11                      3                       3
      DB12                      4                       4
      DB13                      5                       5
      DB14                      6                       6
      DB15                      7                       7
      
***************************************/
  DDRD = DDRD | B11111111;  // SET PORT D AS OUTPUT

  _pinRST = portOutputRegister(digitalPinToPort(_rst));
  _bitmaskRST = digitalPinToBitMask(_rst);
  _pinCS = portOutputRegister(digitalPinToPort(_cs));
  _bitmaskCS = digitalPinToBitMask(_cs);
  _pinWR = portOutputRegister(digitalPinToPort(_wr));
  _bitmaskWR = digitalPinToBitMask(_wr);
  _pinDC = portOutputRegister(digitalPinToPort(_dc));
  _bitmaskDC = digitalPinToBitMask(_dc);
	
  pinMode(_rst, OUTPUT);	
  pinMode(_cs, OUTPUT);
  pinMode(_wr, OUTPUT);
  pinMode(_dc,OUTPUT);
#elif defined (TOOLCHAIN_ARM_MICRO)
  _pinRST = new DigitalOut((PinName)_rst);
  _pinCS = new DigitalOut((PinName)_cs);
  _pinWR = new DigitalOut((PinName)_wr);
  _pinDC = new DigitalOut((PinName)_dc);
#endif
#ifdef LPC15XX_H
  _virtualPortD = new BusOut(D0, D1, D2, D3, D4, P0_11, D6, D7);
#elif defined (TOOLCHAIN_ARM_MICRO)
  _virtualPortD = new BusOut(D0, D1, D2, D3, D4, D5, D6, D7);
#endif

  sbi(_pinRST, _bitmaskRST);
  delay(5); 
  cbi(_pinRST, _bitmaskRST);
  delay(15);
  sbi(_pinRST, _bitmaskRST);
  sbi(_pinCS, _bitmaskCS);
  sbi(_pinWR, _bitmaskWR);
  delay(15);
  cbi(_pinCS, _bitmaskCS);
	
  // Power up sequence
  sendCommand(0x11); sendData(0x001A);            // S6D0164 INIT		
  sendCommand(0x12); sendData(0x3121);
  sendCommand(0x13); sendData(0x006C);
  sendCommand(0x14); sendData(0x4249);
  sendCommand(0x10); sendData(0x0800);
  delay(10);
  sendCommand(0x11); sendData(0x011A);
  delay(10);
  sendCommand(0x11); sendData(0x031A);
  delay(10);
  sendCommand(0x11); sendData(0x071A);
  delay(10);
  sendCommand(0x11); sendData(0x0F1A);
  delay(20);
  sendCommand(0x11); sendData(0x0F3A);
  delay(30);
  // Initialization set sequence
  sendCommand(0x01); sendData(0x011C);
  sendCommand(0x02); sendData(0x0100);
  sendCommand(0x03); sendData(0x1030);
  sendCommand(0x07); sendData(0x0000);
  sendCommand(0x08); sendData(0x0808);
  sendCommand(0x0B); sendData(0x1100);
  sendCommand(0x0C); sendData(0x0000);
  sendCommand(0x0F); sendData(0x1401);
  sendCommand(0x15); sendData(0x0000);
  sendCommand(0x20); sendData(0x0000);
  sendCommand(0x21); sendData(0x0000);
	
  sendCommand(0x38); sendData(0x00DB);
  sendCommand(0x39); sendData(0x0000);
  sendCommand(0x50); sendData(0x0001);
  sendCommand(0x51); sendData(0x020B);
  sendCommand(0x52); sendData(0x0805);	
  sendCommand(0x53); sendData(0x0404);
  sendCommand(0x54); sendData(0x0C0C);
  sendCommand(0x55); sendData(0x000C);
  sendCommand(0x56); sendData(0x0101);
  sendCommand(0x57); sendData(0x0400);	
  sendCommand(0x58); sendData(0x1108);
  sendCommand(0x59); sendData(0x050C);
  sendCommand(0x36); sendData(0x00AF);
  sendCommand(0x37); sendData(0x0000);
  sendCommand(0x38); sendData(0x00DB);	
  sendCommand(0x39); sendData(0x0000);
  sendCommand(0x0F); sendData(0x0B01);
  sendCommand(0x07); sendData(0x0016);
  delay(2);
  sendCommand(0x07); sendData(0x0017);
  sendCommand(0x22); 
  delay(10);	
  sbi(_pinCS, _bitmaskCS);
	
  fillScreen();
}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
