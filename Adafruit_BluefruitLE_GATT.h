#ifndef ADAFRUIT_BLUEFRUITLE_GATT_H_INCLUDED
  #define ADAFRUIT_BLUEFRUITLE_GATT_H_INCLUDED

  #include "Adafruit_BluefruitLE_SPI.h"
  
  typedef enum {
    CHAR_PROP_NONE = 0,                  // 1
    CHAR_PROP_READ = 0x02,               // 2
    CHAR_PROP_WRITE_NO_RESPONSE = 0x04,  // 3
    CHAR_PROP_WRITE = 0x08,              // 4
    CHAR_PROP_NOTIFY = 0x10,             // 5
    CHAR_PROP_INDICATE = 0x20            // 6
  } CharacteristicPropertyEnum;

  // bitwise OR combination ("|") of CharacteristicPropertyEnum(s):
  typedef unsigned short CharacteristicProperties;
  
  class Adafruit_BluefruitLE_GATT : public Adafruit_BluefruitLE_SPI {

    public:
      void assertOK(boolean condition, const __FlashStringHelper*err);
      
      Adafruit_BluefruitLE_GATT(int8_t csPin, int8_t irqPin, int8_t rstPin = -1) : Adafruit_BluefruitLE_SPI(csPin, irqPin, rstPin) { };

      void setGattDeviceName(const char *name);
      
      int16_t addGattService(const char *uuid128);
      
      int16_t addGattCharacteristic(uint16_t uuid16, CharacteristicProperties props, byte minLen, byte maxLen);
      
      void setGattCharacteristicValue(int16_t id, byte *value, uint16_t len);

      /*
       * Returns the number of bytes in reply; bytes exceeding maxLen will be read but will not be returned.as reply.
       */
      uint16_t getGattCharacteristicValue(int16_t id, byte *reply, uint16_t maxLen);
      
    protected:
    
      /*
       * Reply will always be '\0'-terminated; result characters exceeding maxLen will be read but not returned.
       */
      bool sendCommandWithStringReply(const char cmd[], char *reply, uint16_t *len);
  };

#endif
