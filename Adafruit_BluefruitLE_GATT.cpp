
#include <stdio.h>
#include "Adafruit_BluefruitLE_GATT.h"

const char fmt_gapdevname[]     PROGMEM = "AT+GAPDEVNAME=%s";
const char fmt_gattaddservice[] PROGMEM = "AT+GATTADDSERVICE=UUID128=%s";
const char fmt_gattaddchar[]    PROGMEM = "AT+GATTADDCHAR=UUID=%#4X,PROPERTIES=%#2X,MIN_LEN=%i,MAX_LEN=%i,VALUE=0";
const char fmt_gattsetchar[]    PROGMEM = "AT+GATTCHAR=%i,%s";
const char fmt_gattgetchar[]    PROGMEM = "AT+GATTCHAR=%i";
const char fmt_bintohex[]       PROGMEM = "%02X-";

void Adafruit_BluefruitLE_GATT::assertOK(boolean condition, const __FlashStringHelper*err) {
  if (! condition) {
    Serial.print("### ");
    Serial.println(err);
    Serial.flush();
    while(1);
  }
}

void Adafruit_BluefruitLE_GATT::setGattDeviceName(const char *name) {
  char cmd[strlen_P(fmt_gapdevname) - 2 + strlen(name) + 1];
  sprintf_P(cmd, fmt_gapdevname, name);
  assertOK(sendCommandCheckOK(cmd), F("Could not set device name?"));
}

int16_t Adafruit_BluefruitLE_GATT::addGattService(const char *uuid128) {
  char cmd[strlen_P(fmt_gattaddservice) - 2 + strlen(uuid128) + 1];
  sprintf_P(cmd, fmt_gattaddservice, uuid128);
  int32_t pos;
  assertOK(sendCommandWithIntReply(cmd, &pos), F("Could not add service"));
  return (int16_t) pos;
}

int16_t Adafruit_BluefruitLE_GATT::addGattCharacteristic(uint16_t uuid16, CharacteristicProperties props, byte minLen, byte maxLen) {
  char cmd[strlen_P(fmt_gattaddchar) + 2 + 1 + 1 + 1];
  sprintf_P(cmd, fmt_gattaddchar, uuid16, props, (uint16_t) minLen, (uint16_t) maxLen);
  int32_t pos;
  assertOK(sendCommandWithIntReply(cmd, &pos), F("Could not add characteristic"));
  return (int16_t) pos;
}

void Adafruit_BluefruitLE_GATT::setGattCharacteristicValue(int16_t id, byte *value, uint16_t len) {
  assertOK(id != 0, F("Characteristic id cannot be 0"));
  assertOK(len != 0, F("Characteristic value length cannot be 0"));
  
  // AT+GATTCHAR takes each byte in hex separated by a dash, e.g. 4 bytes: xx-xx-xx-xx (= 11 characters)
  char str[len*3];  // includes terminating '\0'
  for (uint16_t i=0; i<len; i++) {
    sprintf_P(&str[i*3], fmt_bintohex, value[i]);
  }
  str[len*3 - 1] = '\0';
  
  char cmd[strlen_P(fmt_gattsetchar) + strlen(str) + 1];
  sprintf_P(cmd, fmt_gattsetchar, id, str);
  assertOK(sendCommandCheckOK(cmd), F("Could not set characteristic value"));
}

uint16_t Adafruit_BluefruitLE_GATT::getGattCharacteristicValue(int16_t id, byte *reply, uint16_t maxLen) {
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


