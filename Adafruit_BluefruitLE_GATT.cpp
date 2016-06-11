
#include <stdio.h>
#include "Adafruit_BluefruitLE_GATT.h"

#define RED_LED_PIN 13

const char fmt_gapdevname[]     PROGMEM = "AT+GAPDEVNAME=%s";
const char fmt_gattaddservice[] PROGMEM = "AT+GATTADDSERVICE=UUID128=%s";
const char fmt_gattaddchar[]    PROGMEM = "AT+GATTADDCHAR=UUID=%#4X,PROPERTIES=%#2X,MIN_LEN=%i,MAX_LEN=%i,VALUE=%s";
const char fmt_gattsetchar[]    PROGMEM = "AT+GATTCHAR=%i,%s";
const char fmt_gattgetchar[]    PROGMEM = "AT+GATTCHAR=%i";
const char fmt_bintohex[]       PROGMEM = "%02X-";

void Adafruit_BluefruitLE_GATT::assertOK(boolean condition, const __FlashStringHelper*err) {
  if (condition) return;
  
  Serial.print(F("### S.O.S. ### "));
  Serial.println(err);
  Serial.flush();
  int pulse = 300; // [ms]
  while(1) {
    // S.O.S.
    pulse = (pulse + 200) % 400; // toggles between 100 and 300 ms
    delay(pulse);
    for(byte i=0; i<3; i++) {
      digitalWrite(RED_LED_PIN, HIGH);
      delay(pulse);
      digitalWrite(RED_LED_PIN, LOW);
      delay(pulse); 
    }      
  }
}

void Adafruit_BluefruitLE_GATT::setGattDeviceName(const char *name) {
  char cmd[strlen_P(fmt_gapdevname) + strlen(name) + 1];
  sprintf_P(cmd, fmt_gapdevname, name);
  assertOK(sendCommandCheckOK(cmd), F("Could not set device name?"));
}


int8_t Adafruit_BluefruitLE_GATT::addGattService(const char *uuid128) {
  char cmd[strlen_P(fmt_gattaddservice) + strlen(uuid128) + 1];
  sprintf_P(cmd, fmt_gattaddservice, uuid128);
  int32_t pos;
  assertOK(sendCommandWithIntReply(cmd, &pos), F("Could not add service"));
  return (int8_t) pos;
}


int8_t Adafruit_BluefruitLE_GATT::addGattCharacteristic(uint16_t uuid16, CharacteristicProperties props, uint8_t minLen, uint8_t maxLen) {
  char zeros[maxLen*3];
  for(uint8_t i=0; i<maxLen; i++) {
    zeros[i*3]   = '0';
    zeros[i*3+1] = '0';
    zeros[i*3+2] = '-';
  }
  zeros[maxLen*3-1] = '\0';  // replace the last '-' by '\0'
  
  char cmd[strlen_P(fmt_gattaddchar) + 2 + 1 + 1 + maxLen*3];
  sprintf_P(cmd, fmt_gattaddchar, uuid16, props, minLen, maxLen, zeros);
  int32_t pos;
  assertOK(sendCommandWithIntReply(cmd, &pos), F("Could not add characteristic"));
  return (int8_t) pos;
}


void Adafruit_BluefruitLE_GATT::setGattCharacteristicValue(int8_t id, byte *value, uint16_t len) {
  if (len == 0) return;
  
  // AT+GATTCHAR takes each byte in hex separated by a dash, e.g. 4 bytes: xx-xx-xx-xx (= 11 characters)
  char str[len*3 + 1];  // +1 caters for terminating '\0' after each 'xx-' group
  for (uint16_t i=0; i<len; i++) {
    sprintf_P(&str[i*3], fmt_bintohex, value[i]);
  }
  str[len*3 - 1] = '\0'; // replace the last '-' by '\0'
  
  char cmd[strlen_P(fmt_gattsetchar) + len*3];
  sprintf_P(cmd, fmt_gattsetchar, id, str);
  assertOK(sendCommandCheckOK(cmd), F("Could not set characteristic value"));
}


void Adafruit_BluefruitLE_GATT::setGattCharacteristicValue(int8_t id, int16_t value) {
  byte bytes[sizeof(int16_t)];
  memcpy(bytes, &value, sizeof(int16_t));
  reverseBytes(bytes, sizeof(int16_t));
  setGattCharacteristicValue(id, bytes, sizeof(int16_t)); 
}


void Adafruit_BluefruitLE_GATT::setGattCharacteristicValue(int8_t id, int32_t value) {
  byte bytes[sizeof(int32_t)];
  memcpy(bytes, &value, sizeof(int32_t));
  reverseBytes(bytes, sizeof(int32_t));
  setGattCharacteristicValue(id, bytes, sizeof(int32_t)); 
}


void Adafruit_BluefruitLE_GATT::setGattCharacteristicValue(int8_t id, float value) {
  byte bytes[sizeof(float)];
  memcpy(bytes, &value, sizeof(float));
  setGattCharacteristicValue(id, bytes, sizeof(float)); 
}


uint16_t Adafruit_BluefruitLE_GATT::getGattCharacteristicValue(int8_t id, byte *reply, uint16_t maxLen) {
  char cmd[strlen_P(fmt_gattgetchar) + 2 + 1];
  sprintf_P(cmd, fmt_gattgetchar, id);
  
  // AT+GATTCHAR returns each byte in hex separated by a dash, e.g. 4 bytes: xx-xx-xx-xx (= 11 characters)
  char replyStr[maxLen*3];  // includes terminating '\0'
  uint16_t strLen;
  assertOK(sendCommandWithStringReply(cmd, replyStr, &strLen), F("Could not get characteristic value"));
  
  if (strLen == 0 || (strLen + 1) % 3 != 0) {
    return 0;
  }

  // Parse dash-separated hex string into bytes:
  uint16_t numBytes = min((strLen + 1) / 3, maxLen);
  for(uint16_t i=0; i<numBytes; i++) {
    reply[i] = (byte) strtol(&replyStr[i*3], NULL, 16);
  }
  return numBytes;
}


void Adafruit_BluefruitLE_GATT::getGattCharacteristicValue(int8_t id, int16_t *reply) {
  byte bytes[sizeof(int16_t)];
  getGattCharacteristicValue(id, bytes, sizeof(int16_t));
  reverseBytes(bytes, sizeof(int16_t));
  memcpy(reply, bytes, sizeof(int16_t));
}


void Adafruit_BluefruitLE_GATT::getGattCharacteristicValue(int8_t id, int32_t *reply) {
  byte bytes[sizeof(int32_t)];
  getGattCharacteristicValue(id, bytes, sizeof(int32_t));
  reverseBytes(bytes, sizeof(int32_t));
  memcpy(reply, bytes, sizeof(int32_t));
}


void Adafruit_BluefruitLE_GATT::getGattCharacteristicValue(int8_t id, float   *reply) {
  byte bytes[sizeof(float)];
  getGattCharacteristicValue(id, bytes, sizeof(float));
  // don't reverse bytes!
  memcpy(reply, bytes, sizeof(float));
}


bool Adafruit_BluefruitLE_GATT::sendCommandWithStringReply(const char cmd[], char *reply, uint16_t *len) {
  bool success;
  uint8_t current_mode = _mode;

  // switch mode if necessary to execute command
  if (current_mode == BLUEFRUIT_MODE_DATA ) setMode(BLUEFRUIT_MODE_COMMAND);

  println(cmd);                   // send command
  if (_verbose) {
    SerialDebug.print( F("\n<- ") );
  }
  (*len) = readline();
  memcpy(reply, buffer, *len);
  success = waitForOK();

  // switch back if necessary
  if (current_mode == BLUEFRUIT_MODE_DATA ) setMode(BLUEFRUIT_MODE_DATA);

  return success;
}


void reverseBytes(byte *buf, uint16_t len) {
  for(uint16_t i=0; i<len/2; i++) {
    byte b = buf[i];
    buf[i] = buf[len-1-i];
    buf[len-1-i] = b;
  }
}
