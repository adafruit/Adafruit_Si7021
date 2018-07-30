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

    @section  HISTORY

    v1.0  - First release
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



/**************************************************************************/

Adafruit_Si7021::Adafruit_Si7021(void) {
  _i2caddr = SI7021_DEFAULT_ADDRESS;
  sernum_a = sernum_b = 0;
  model = "Si7021";
}

bool Adafruit_Si7021::begin(void) {
  Wire.begin();

  reset();
  if (readRegister8(SI7021_READRHT_REG_CMD) != 0x3A)
		return false;

  readSerialNumber();
  revision = readRevision();

  return true;
}

float Adafruit_Si7021::readHumidity(void) {
  Wire.beginTransmission(_i2caddr);
  Wire.write(SI7021_MEASRH_NOHOLD_CMD);
  uint8_t err = Wire.endTransmission(false);
#ifdef ARDUINO_ARCH_ESP32
  if(err != I2C_ERROR_CONTINUE) //ESP32 has to queue ReSTART operations.
#else
  if (err != 0)
#endif
		return NAN; //error

  uint32_t start = millis(); // start timeout
  while(millis()-start < TRANSACTION_TIMEOUT){
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
  uint8_t err = Wire.endTransmission(false);

#ifdef ARDUINO_ARCH_ESP32
  if (err != I2C_ERROR_CONTINUE) //ESP32 has to queue ReSTART operations.
#else
  if(err != 0)
#endif
  	return NAN; //error
    
  uint32_t start = millis(); // start timeout
  while(millis()-start < TRANSACTION_TIMEOUT){
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


uint8_t Adafruit_Si7021::readRevision(void)
{
    Wire.beginTransmission(_i2caddr);
    Wire.write((uint8_t)(SI7021_FIRMVERS_CMD >> 8));
    Wire.write((uint8_t)(SI7021_FIRMVERS_CMD & 0xFF));
    Wire.endTransmission();
    
    Wire.requestFrom(_i2caddr, 2);
    int rev = Wire.read();
    Wire.read();
    
    if (rev == SI7021_REV_1) {
        rev = 1;
    } else if (rev == SI7021_REV_2) {
        rev = 2;
    }
    
    return rev;
}

void Adafruit_Si7021::readSerialNumber(void) {
  Wire.beginTransmission(_i2caddr);
  Wire.write((uint8_t)(SI7021_ID1_CMD >> 8));
  Wire.write((uint8_t)(SI7021_ID1_CMD & 0xFF));
  Wire.endTransmission();

  Wire.requestFrom(_i2caddr, 8);
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

  Wire.requestFrom(_i2caddr, 8);
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
            this->model = "SI engineering samples";
            break;
        case 0x0D:
            this->model = "Si7013";
            break;
        case 0x14:
            this->model = "Si7020";
            break;
        case 0x15:
            this->model = "Si7021";
            break;
        default:
            this->model = "unknown SI sensor";
    }    
}

/*******************************************************************/

void Adafruit_Si7021::writeRegister8(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(_i2caddr);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();

  //Serial.print("Wrote $"); Serial.print(reg, HEX); Serial.print(": 0x"); Serial.println(value, HEX);
}

uint8_t Adafruit_Si7021::readRegister8(uint8_t reg) {
  uint8_t value;
  Wire.beginTransmission(_i2caddr);
  Wire.write(reg);
  Wire.endTransmission(false);

  uint32_t start = millis(); // start timeout
  while(millis()-start < TRANSACTION_TIMEOUT){
	  if (Wire.requestFrom(_i2caddr, 1) == 1) {
		  value = Wire.read();
		  return value;
    }
    delay(2);
  }

  return 0; // Error timeout
}

uint16_t Adafruit_Si7021::readRegister16(uint8_t reg) {
  uint16_t value;
  Wire.beginTransmission(_i2caddr);
  Wire.write(reg);
  Wire.endTransmission(false);

  uint32_t start = millis(); // start timeout
  while(millis()-start < TRANSACTION_TIMEOUT){
	  if (Wire.requestFrom(_i2caddr, 2) == 2) {
      value = Wire.read() << 8 | Wire.read();
			return value;
    }
    delay(2);
  }
  return 0; // Error timeout
}
