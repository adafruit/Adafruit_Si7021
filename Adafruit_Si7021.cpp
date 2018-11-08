/**************************************************************************/
/*!
    @file     Adafruit_Si7021.cpp
    @author   Limor Fried (Adafruit Industries)
    @license  BSD (see license.txt)

    This is a library for the Adafruit Si7021 breakout board
    ----> https://www.adafruit.com/products/3251

    Adafruit invests time and resources providing this open source code,
    please support Adafruit and open-source hardware by purchasing
    products from Adafruit!
*/
/**************************************************************************/

#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#include <Wire.h>
#include <Adafruit_Si7021.h>


/**************************************************************************/

Adafruit_Si7021::Adafruit_Si7021(void) {
  _i2caddr = SI7021_DEFAULT_ADDRESS;
  sernum_a = sernum_b = 0;
  _model = SI_7021;
  _revision = 0;
}

bool Adafruit_Si7021::begin(void) {
  Wire.begin();

  reset();
  if (_readRegister8(SI7021_READRHT_REG_CMD) != 0x3A)
    return false;

  readSerialNumber();
  _readRevision();

  return true;
}

float Adafruit_Si7021::readHumidity(void) {
  Wire.beginTransmission(_i2caddr);

  Wire.write(SI7021_MEASRH_NOHOLD_CMD);
  uint8_t err = Wire.endTransmission();
  if (err != 0)
    return NAN; //error

  uint32_t start = millis(); // start timeout
  while(millis()-start < _TRANSACTION_TIMEOUT) {
    if (Wire.requestFrom(_i2caddr, 3) == 3) {
      uint16_t hum = Wire.read() << 8 | Wire.read();
      uint8_t chxsum = Wire.read();

      float humidity = hum;
      humidity *= 125;
      humidity /= 65536;
      humidity -= 6;

      return humidity;
    }
    delay(6); // 1/2 typical sample processing time
  }
  return NAN; // Error timeout
}

float Adafruit_Si7021::readTemperature(void) {
  Wire.beginTransmission(_i2caddr);
  Wire.write(SI7021_MEASTEMP_NOHOLD_CMD);
  uint8_t err = Wire.endTransmission();

  if(err != 0)
    return NAN; //error
    
  uint32_t start = millis(); // start timeout
  while(millis()-start < _TRANSACTION_TIMEOUT) {
    if (Wire.requestFrom(_i2caddr, 3) == 3) {
      uint16_t temp = Wire.read() << 8 | Wire.read();
      uint8_t chxsum = Wire.read();

      float temperature = temp;
      temperature *= 175.72;
      temperature /= 65536;
      temperature -= 46.85;
      return temperature;
    }
    delay(6); // 1/2 typical sample processing time
  }

  return NAN; // Error timeout
}

void Adafruit_Si7021::reset(void) {
  Wire.beginTransmission(_i2caddr);
  Wire.write(SI7021_RESET_CMD);
  Wire.endTransmission();
  delay(50);
}


void Adafruit_Si7021::_readRevision(void)
{
    Wire.beginTransmission(_i2caddr);
    Wire.write((uint8_t)(SI7021_FIRMVERS_CMD >> 8));
    Wire.write((uint8_t)(SI7021_FIRMVERS_CMD & 0xFF));
    Wire.endTransmission();
    
    uint32_t start = millis(); // start timeout
    while(millis()-start < _TRANSACTION_TIMEOUT) {
      if (Wire.requestFrom(_i2caddr, 2) == 2) {
        uint8_t rev = Wire.read();
        Wire.read();
    
        if (rev == SI7021_REV_1) {
          rev = 1;
        } else if (rev == SI7021_REV_2) {
          rev = 2;
        }
        _revision = rev;
        return;
      }
      delay(2);
    }
    _revision = 0;
    return; // Error timeout
}

void Adafruit_Si7021::readSerialNumber(void) {
  Wire.beginTransmission(_i2caddr);
  Wire.write((uint8_t)(SI7021_ID1_CMD >> 8));
  Wire.write((uint8_t)(SI7021_ID1_CMD & 0xFF));
  Wire.endTransmission();

  bool gotData = false;
  uint32_t start = millis(); // start timeout
  while(millis()-start < _TRANSACTION_TIMEOUT) {
    if (Wire.requestFrom(_i2caddr, 8) == 8) {
      gotData = true;
      break;
    }
    delay(2);
  }
  if (!gotData)
    return; // error timeout

  sernum_a = Wire.read();
  Wire.read();
  sernum_a <<= 8;
  sernum_a |= Wire.read();
  Wire.read();
  sernum_a <<= 8;
  sernum_a |= Wire.read();
  Wire.read();
  sernum_a <<= 8;
  sernum_a |= Wire.read();
  Wire.read();

  Wire.beginTransmission(_i2caddr);
  Wire.write((uint8_t)(SI7021_ID2_CMD >> 8));
  Wire.write((uint8_t)(SI7021_ID2_CMD & 0xFF));
  Wire.endTransmission();

  gotData = false;
  start = millis(); // start timeout
  while(millis()-start < _TRANSACTION_TIMEOUT){
    if (Wire.requestFrom(_i2caddr, 8) == 8) {
      gotData = true;
      break;
    }
    delay(2);
  }
  if (!gotData)
    return; // error timeout

  sernum_b = Wire.read();
  Wire.read();
  sernum_b <<= 8;
  sernum_b |= Wire.read();
  Wire.read();
  sernum_b <<= 8;
  sernum_b |= Wire.read();
  Wire.read();
  sernum_b <<= 8;
  sernum_b |= Wire.read();
  Wire.read();

  switch(sernum_b >> 24) {
    case 0:
    case 0xff:
      _model = SI_Engineering_Samples;
        break;
    case 0x0D:
      _model = SI_7013;
      break;
    case 0x14:
      _model = SI_7020;
      break;
    case 0x15:
      _model = SI_7021;
      break;
    default:
      _model = SI_UNKNOWN;
    }    
}

si_sensorType Adafruit_Si7021::getModel(void)
{
  return _model;
}

/*******************************************************************/

void Adafruit_Si7021::_writeRegister8(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(_i2caddr);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();

  //Serial.print("Wrote $"); Serial.print(reg, HEX); Serial.print(": 0x"); Serial.println(value, HEX);
}

uint8_t Adafruit_Si7021::_readRegister8(uint8_t reg) {
  uint8_t value;
  Wire.beginTransmission(_i2caddr);
  Wire.write((uint8_t)reg);
  Wire.endTransmission();

  uint32_t start = millis(); // start timeout
  while(millis()-start < _TRANSACTION_TIMEOUT) {
    if (Wire.requestFrom(_i2caddr, 1) == 1) {
      value = Wire.read();
      return value;
    }
    delay(2);
  }

  return 0; // Error timeout
}

uint16_t Adafruit_Si7021::_readRegister16(uint8_t reg) {
  uint16_t value;
  Wire.beginTransmission(_i2caddr);
  Wire.write(reg);
  Wire.endTransmission(false);

  uint32_t start = millis(); // start timeout
  while(millis()-start < _TRANSACTION_TIMEOUT) {
    if (Wire.requestFrom(_i2caddr, 2) == 2) {
      value = Wire.read() << 8 | Wire.read();
      return value;
    }
    delay(2);
  }
  return 0; // Error timeout
}
