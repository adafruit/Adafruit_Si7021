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
    v1.1  - async Wire fixes to work with the ESP32 (thanks to stickbreaker)
            New features to get the sensor revision and model.
*/
/**************************************************************************/

#ifndef __Si7021_H__
#define __Si7021_H__

#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

/*=========================================================================
    I2C ADDRESS/BITS
    -----------------------------------------------------------------------*/
#define SI7021_DEFAULT_ADDRESS	0x40

#define SI7021_MEASRH_HOLD_CMD           0xE5 // Measure Relative Humidity, Hold Master Mode
#define SI7021_MEASRH_NOHOLD_CMD         0xF5 // Measure Relative Humidity, No Hold Master Mode
#define SI7021_MEASTEMP_HOLD_CMD         0xE3 // Measure Temperature, Hold Master Mode
#define SI7021_MEASTEMP_NOHOLD_CMD       0xF3 // Measure Temperature, No Hold Master Mode
#define SI7021_READPREVTEMP_CMD          0xE0 // Read Temperature Value from Previous RH Measurement
#define SI7021_RESET_CMD                 0xFE
#define SI7021_WRITERHT_REG_CMD          0xE6 // Write RH/T User Register 1
#define SI7021_READRHT_REG_CMD           0xE7 // Read RH/T User Register 1
#define SI7021_WRITEHEATER_REG_CMD       0x51 // Write Heater Control Register
#define SI7021_READHEATER_REG_CMD        0x11 // Read Heater Control Register
#define SI7021_ID1_CMD                   0xFA0F // Read Electronic ID 1st Byte
#define SI7021_ID2_CMD                   0xFCC9 // Read Electronic ID 2nd Byte
#define SI7021_FIRMVERS_CMD              0x84B8 // Read Firmware Revision

#define SI7021_REV_1					0xff
#define SI7021_REV_2					0x20

/*=========================================================================*/

enum si_sensorType {
  SI_Engineering_Samples,
  SI_7013,
  SI_7020,
  SI_7021,
  SI_UNKNOWN,
};

class Adafruit_Si7021 {
 public:
  Adafruit_Si7021(void);
  bool begin(void);

  float readTemperature(void);
  void reset(void);
  void readSerialNumber(void);
  float readHumidity(void);

  uint8_t getRevision(void) { return _revision; };
  uint32_t sernum_a, sernum_b;


  si_sensorType getModel(void);

 private:
  void _readRevision(void);
  si_sensorType _model;
  uint8_t _revision;
  uint8_t _readRegister8(uint8_t reg);
  uint16_t _readRegister16(uint8_t reg);
  void _writeRegister8(uint8_t reg, uint8_t value);

  int8_t  _i2caddr;
  const static int _TRANSACTION_TIMEOUT = 100; // Wire NAK/Busy timeout in ms

};

/**************************************************************************/

#endif // __Si7021_H__
