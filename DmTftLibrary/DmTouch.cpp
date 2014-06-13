/**********************************************************************************************
 Copyright (c) 2014 DisplayModule. All rights reserved.

 Redistribution and use of this source code, part of this source code or any compiled binary
 based on this source code is permitted as long as the above copyright notice and following
 disclaimer is retained.

 DISCLAIMER:
 THIS SOFTWARE IS SUPPLIED "AS IS" WITHOUT ANY WARRANTIES AND SUPPORT. DISPLAYMODULE ASSUMES
 NO RESPONSIBILITY OR LIABILITY FOR THE USE OF THE SOFTWARE.
 ********************************************************************************************/
// Tested with Xpt2046

#include "DmTouch.h"
#define MEASUREMENTS 10

// disp        - which display is used
// spiMode     - How to read SPI-data, Software, Hardware or Auto
// useIrq      - Enable IRQ or disable IRQ
DmTouch::DmTouch(Display disp, SpiMode spiMode, bool useIrq)
{
  switch (disp) {
    // Display with 40-pin connector on top of adapter board
    case DmTouch::DM_TFT28_103:
    case DmTouch::DM_TFT24_104:
      _cs = D8;
      _irq = D10;
      _clk = A1;
      _mosi = A0;
      _miso = D9;
	  _hardwareSpi = false;
      break;
    
    case DmTouch::DM_TFT28_105:
      _cs = D4;
      _irq = D2;
      _clk = D13;
      _mosi = D11;
      _miso = D12;
	  _hardwareSpi = true;
      break;

    case DmTouch::DM_TFT35_107:
    default:
      _cs = D4;
      _irq = D2;
      _clk = D13;
      _mosi = D11;
      _miso = D12;
	  _hardwareSpi = true;
      break;
  }
  
  if (spiMode == DmTouch::Hardware) {
	_hardwareSpi = true;
  } else if (spiMode == DmTouch::Software) {
	_hardwareSpi = false;
  }
  
  if (!useIrq) {
	_irq = -1;
  }

  _samplesPerMeasurement = 3;
}

void DmTouch::init() {
#if defined (DM_TOOLCHAIN_ARDUINO)
  pinMode(_cs, OUTPUT);
  _pinCS  = portOutputRegister(digitalPinToPort(_cs));
  _bitmaskCS  = digitalPinToBitMask(_cs);

  if (_hardwareSpi) {
    SPI.begin();
    SPI.setClockDivider(SPI_CLOCK_DIV32);
    SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE0);
    _spiSettings = SPCR;
  }
  else {
    pinMode(_clk, OUTPUT);
    pinMode(_mosi, OUTPUT);
    pinMode(_miso, INPUT);
    _pinCLK  = portOutputRegister(digitalPinToPort(_clk));
    _bitmaskCLK  = digitalPinToBitMask(_clk);
    _pinMOSI  = portOutputRegister(digitalPinToPort(_mosi));
    _bitmaskMOSI  = digitalPinToBitMask(_mosi);
    _pinMISO = portInputRegister(digitalPinToPort(_miso));
    _bitmaskMISO  = digitalPinToBitMask(_miso);
  }
#elif defined (DM_TOOLCHAIN_MBED)
  _pinCS = new DigitalOut((PinName)_cs);
  if (_hardwareSpi) {
    sbi(_pinCS, _bitmaskCS);
    _spi = new SPI((PinName)_mosi, (PinName)_miso, (PinName)_clk);
    _spi->format(8,0);
    _spi->frequency(2000000); // Max SPI speed
    //cbi(_pinCS, _bitmaskCS);
  } else {
    _pinCLK = new DigitalOut((PinName)_clk);
    _pinMISO = new DigitalOut((PinName)_miso);
    _pinMOSI = new DigitalOut((PinName)_mosi);
  }
#endif

  if (_irq != -1) { // We will use Touch IRQ
    enableIrq();
  }
}

void DmTouch::enableIrq() {
#if defined (DM_TOOLCHAIN_ARDUINO)
  pinMode(_irq, INPUT);
  _pinIrq = portInputRegister(digitalPinToPort(_irq));
  _bitmaskIrq  = digitalPinToBitMask(_irq);
#elif defined (DM_TOOLCHAIN_MBED)
  _pinIrq = new DigitalIn((PinName)_irq);
  _pinIrq->mode(PullUp);
#endif

  cbi(_pinCS, _bitmaskCS);
  spiWrite(0x80); // Enable PENIRQ
  sbi(_pinCS, _bitmaskCS);
}

void DmTouch::spiWrite(uint8_t data) {
  if (_hardwareSpi) {
#if defined (DM_TOOLCHAIN_ARDUINO)
    SPCR = _spiSettings;   // SPI Control Register
    SPDR = data;        // SPI Data Register
    while(!(SPSR & _BV(SPIF)));  // SPI Status Register Wait for transmission to finish
#elif defined (DM_TOOLCHAIN_MBED)
    _spi->write(data);
#endif
  }
  else {
    uint8_t count=0;
    uint8_t temp = data;
    delay(1);
	cbi(_pinCLK, _bitmaskCLK);
    for(count=0;count<8;count++) {
      if(temp&0x80) {
        sbi(_pinMOSI, _bitmaskMOSI);
      }
      else {
        cbi(_pinMOSI, _bitmaskMOSI);
      }

      temp=temp<<1;

      pulse_low(_pinCLK, _bitmaskCLK);
    }
  }
}

uint8_t DmTouch::spiRead() {// Only used for Hardware SPI
#if defined (DM_TOOLCHAIN_ARDUINO)
  uint8_t data;
  SPCR = _spiSettings;
  spiWrite(0x00);
  data = SPDR;

  return data;
#elif defined (DM_TOOLCHAIN_MBED)
  if (_hardwareSpi) {
    return _spi->write(0x00); // dummy byte to read
  } else {
    uint8_t count=0;
    uint8_t temp=0;
    cbi(_pinCLK, _bitmaskCLK);
    cbi(_pinMOSI, _bitmaskMOSI);
    for(count=0;count<8;count++) {
      pulse_low(_pinCLK, _bitmaskCLK);
      temp = temp<<1;
      temp |= _pinMISO->read();
    }
    return temp;
  }
#endif
}

uint16_t DmTouch::readData12(uint8_t command) {
  uint8_t temp = 0;
  uint16_t value = 0;

  spiWrite(command); // Send command
  delayMicroseconds(10); // Test

#if defined (DM_TOOLCHAIN_ARDUINO)
  if (_hardwareSpi) {
    // We use 7-bits from the first byte and 5-bit from the second byte
    temp = spiRead();

    value = temp<<8;
    temp = spiRead();

	value |= temp;
    value >>=3;
    value &= 0xFFF;
  } else {
    pulse_high(_pinCLK, _bitmaskCLK);
    unsigned nop;
    uint8_t count=0;
    for(count=0;count<12;count++) {
      value<<=1;
      pulse_high(_pinCLK, _bitmaskCLK);
      if ( gbi(_pinMISO, _bitmaskMISO) ) {
        value++;
      }
    }
  }
#elif defined (DM_TOOLCHAIN_MBED)
  // We use 7-bits from the first byte and 5-bit from the second byte
  temp = spiRead();
  value = temp<<8;
  temp = spiRead();
  value |= temp;
  value >>=3;
  value &= 0xFFF;
#endif
  return value;
}

void DmTouch::readRawData(uint16_t &x, uint16_t &y) {
  cbi(_pinCS, _bitmaskCS);
  x = readData12(0xD0);
  y = readData12(0x90);
  sbi(_pinCS, _bitmaskCS);
}

void DmTouch::readTouchData(uint16_t& posX, uint16_t& posY, bool& touching) {
#if defined (DM_TOOLCHAIN_MBED)
  if (!isTouched()) {
    touching = false;
    return;
  }
#endif
  uint16_t touchX, touchY;
  getMiddleXY(touchX,touchY);  

  posX = getDisplayCoordinateX(touchX, touchY);
  posY = getDisplayCoordinateY(touchX, touchY);
}

bool DmTouch::isSampleValid() {
  uint16_t sampleX,sampleY;
  readRawData(sampleX,sampleY);
  if (sampleX > 0 && sampleX < 4095 && sampleY > 0 && sampleY < 4095) {
    return true;
  } else {
    return false;
  }
}

bool DmTouch::isTouched() {
#if defined (DM_TOOLCHAIN_ARDUINO)
  if (_irq == -1) {
    return isSampleValid();
  }

  if ( !gbi(_pinIrq, _bitmaskIrq) ) {
    return true;
  }

  return false;
#elif defined (DM_TOOLCHAIN_MBED)
  return (*_pinIrq == 0);
#endif
}

bool DmTouch::getMiddleXY(uint16_t &x, uint16_t &y) {
  bool haveAllMeasurements  = true;
  uint16_t valuesX[MEASUREMENTS];
  uint16_t valuesY[MEASUREMENTS];
  uint8_t nbrOfMeasurements = 0;
   
  for (int i=0; i<MEASUREMENTS; i++) {
    getAverageXY(valuesX[i], valuesY[i]);
    nbrOfMeasurements++;
    if (!isTouched()) {
      haveAllMeasurements = false;
      break;
    }
  }
  x = calculateMiddleValue(valuesX, nbrOfMeasurements);
  y = calculateMiddleValue(valuesY, nbrOfMeasurements);
   
  return haveAllMeasurements;
}

void DmTouch::getAverageXY(uint16_t &x, uint16_t &y) {
  uint32_t sumX = 0;
  uint32_t sumY = 0;
  uint16_t sampleX,sampleY;
  readRawData(sampleX,sampleY);

  for (int i=0; i<_samplesPerMeasurement; i++) {
	readRawData(sampleX,sampleY);
    sumX += sampleX;
    sumY += sampleY;
  }

  x = (uint32_t)sumX/_samplesPerMeasurement;
  y = (uint32_t)sumY/_samplesPerMeasurement;
}

// Total number of samples = MEASUREMENTS * _samplesPerMeasurement
void DmTouch::setPrecison(uint8_t samplesPerMeasurement) {
  _samplesPerMeasurement = samplesPerMeasurement;
}

void DmTouch::setCalibrationMatrix(CalibrationMatrix calibrationMatrix) {
  _calibrationMatrix = calibrationMatrix;
}

void DmTouch::waitForTouch() {
  while(!isTouched()) {}
}

void DmTouch::waitForTouchRelease() {
  while(isTouched()) {}
}

uint16_t DmTouch::getDisplayCoordinateX(uint16_t x_touch, uint16_t y_touch) {
  uint16_t Xd;
  float temp;
  temp = (_calibrationMatrix.a * x_touch + _calibrationMatrix.b * y_touch + _calibrationMatrix.c) / rescaleFactor();
  Xd = (uint16_t)(temp);
  if (Xd > 60000) {
    Xd = 0;
  }
  return Xd;
}

uint16_t DmTouch::getDisplayCoordinateY(uint16_t x_touch, uint16_t y_touch) {
  uint16_t Yd;
  float temp;
  temp = (_calibrationMatrix.d * x_touch + _calibrationMatrix.e * y_touch + _calibrationMatrix.f) / rescaleFactor();
  Yd = (uint16_t)(temp);
  if (Yd > 60000) {
    Yd = 0;
  }
  return Yd;
}

uint16_t DmTouch::calculateMiddleValue(uint16_t values[], uint8_t count) {
  uint16_t temp;

  for(uint8_t i=0; i<count-1; i++) {
    for(uint8_t j=i+1; j<count; j++) {
      if(values[j] < values[i]) {
        temp = values[i];
        values[i] = values[j];
        values[j] = temp;
      }
    }
  }

  if(count%2==0) {
    return((values[count/2] + values[count/2 - 1]) / 2.0);
  } else {
    return values[count/2];
  }
}


