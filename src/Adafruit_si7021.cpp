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

#define TRANSACTION_TIMEOUT 100 // Wire NAK/Busy timeout in ms


#ifndef ARDUINO_ARCH_ESP32
 #define log_e()
#endif 

/**************************************************************************/

const char MODELNAMES[]={
    "SI engineering samples\0"
    "Si7013\0"
    "Si7020\0"
    "Si7021\0"
    "unknown\0"
    "SHT25\0"
    "\0"
};

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
// load data into array for crc calculation
      uint8_t buf[3];
      buf[0] = Wire.read();
      buf[1] = Wire.read();
      buf[2] = Wire.read();
      if(calcCrc(buf,2) != buf[2]){
          log_e("crc error actual=0x%02X calc=0x%02X",buf[2],calcCrc(buf,2));
          return NAN;
      }
      uint16_t hum = buf[0] << 8 | buf[1];
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
      uint8_t buf[3];
      buf[0] = Wire.read();
      buf[1] = Wire.read();
      buf[2] = Wire.read();
      if(calcCrc(buf,2) != buf[2]){
          log_e("crc error actual=0x%02X calc=0x%02X",buf[2],calcCrc(buf,2));
          return NAN;
      }
      
      uint16_t temp = buf[0] << 8 | buf[1];
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

  //receive data into buffer to simplify CRC calculation
  uint8_t buf[8];
  for(auto a=0;a<8; a++){
      buf[a] = Wire.read();
  }
  sernum_a = 0;
  for(auto a=0; a<4;a++){
      sernum_a <<= 8;
      sernum_a |= buf[a*2]; //skip over checksum bytes
  }
  // test crc
  for(auto a= 0; a<4; a++){
      if(calcCrc((uint8_t*)&buf[a*2],1) != buf[(a*2)+1]){
          log_e("CRC mismatch on byte %d of First Serial number read. actual=0x%02x calc=0x%02x",(3-a),buf[(a*2)+1],
            calcCrc((uint8_t*)&buf[a*2],1));
      }
  }
  
  Wire.beginTransmission(_i2caddr);
  Wire.write((uint8_t)(SI7021_ID2_CMD >> 8));
  Wire.write((uint8_t)(SI7021_ID2_CMD & 0xFF));
  Wire.endTransmission();

  gotData = false;
  start = millis(); // start timeout
  while(millis()-start < _TRANSACTION_TIMEOUT){
    if (Wire.requestFrom(_i2caddr, 6) == 6) {
      gotData = true;
      break;
    }
    delay(2);
  }

  if (!gotData)
    return; // error timeout

  for(auto a=0;a<6; a++){
      buf[a] = Wire.read();
  }
  sernum_b = ((buf[0]<<8 ) | buf[1]);
  if(calcCrc(buf,2) != buf[2]){
      log_e("crc error actual=0x%02X calc=0x%02X",buf[2],calcCrc(buf,2));
  }
  uint16_t x= ((buf[3]<<8) | buf[4]);
  if(calcCrc((uint8_t*)&buf[3],2) != buf[5]){
      log_e("crc error actual=0x%02X calc=0x%02X",buf[5],calcCrc((uint8_t*)&buf[3],2));
  }
  sernum_b = (sernum_b<<16) | x;
  
  switch(sernum_b >> 24) {
    case 0:
    case 0xff:
      _model = SI_Engineering_Samples;
        break;
    case 0x01:
      _model = SHT_25;
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

char * Adafruit_Si7021::getModelText(si_sensorType model){
int pos = 0,count=0;
bool done=false;
bool foundNULL=false;
while((count<model)&& !done){
    if(foundNULL && MODELNAMES[pos]=='\0'){
        done=true;
        break;
    }
    foundNULL = MODELNAMES[pos] == '\0';
    if(foundNULL) count++;
    pos++;
}
return (char*)&MODELNAMES[pos];
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

#define POLYNOMIAL 0x131      //P(x)=x^8+x^5+x^4+1 = 100110001
 uint8_t Adafruit_Si7021::calcCrc (uint8_t data[], uint8_t nbrOfBytes)
 {
/*
 ============================================================ 
 calculates checksum for n bytes of data
 and compares it with expected checksum
 input:    data[]        checksum is built based on this data
          nbrOfBytes    checksum is built for n bytes of data
 return:   crc8 
============================================================ 
*/
    uint8_t crc = 0;
    uint8_t byteCtr; //calculates 8-Bit checksum with given polynomial
    for (byteCtr = 0; byteCtr < nbrOfBytes; ++byteCtr) {
        crc ^= (data[byteCtr]);
        for (uint8_t bit = 8; bit > 0; --bit)   {
            if (crc & 0x80) crc = (crc << 1) ^ POLYNOMIAL;
            else crc = (crc << 1);
        }
    }
    return crc;
 }

