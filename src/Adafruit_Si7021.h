/**************************************************************************/
/*!
    @file     Adafruit_Si7021.h
    @author   Limor Fried (Adafruit Industries)
    @license  BSD (see license.txt)

    This is a library for the Adafruit Si7021 breakout board
    ----> https://www.adafruit.com/products/3251

    Adafruit invests time and resources providing this open source code,
    please support Adafruit and open-source hardware by purchasing
    products from Adafruit!

    @section  HISTORY

    v1.0  - First release
	V1.1.o - Modified for ESP32 by stickbreaker
*/
/**************************************************************************/

#ifndef __Si7021_H__
#define __Si7021_H__

#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

class Adafruit_Si7021 {
 public:
  Adafruit_Si7021(void);
  bool begin(void);

  float readTemperature(void);
  void reset(void);
  void readSerialNumber(void);
  float readHumidity(void);

  uint32_t sernum_a, sernum_b;

 private:

  uint8_t readRegister8(uint8_t reg);
  uint16_t readRegister16(uint8_t reg);
  void writeRegister8(uint8_t reg, uint8_t value);

  int8_t  _i2caddr;
};

/**************************************************************************/

#endif // __Si7021_H__
