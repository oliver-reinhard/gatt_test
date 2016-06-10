
#include <stdio.h>
#include "Adafruit_BluefruitLE_GATT.h"

const char fmt_gapdevname[]     PROGMEM = "AT+GAPDEVNAME=%s";
const char fmt_gattaddservice[] PROGMEM = "AT+GATTADDSERVICE=UUID128=%s";
const char fmt_gattaddchar[]    PROGMEM = "AT+GATTADDCHAR=UUID=%#4X,PROPERTIES=%#2X,MIN_LEN=%i,MAX_LEN=%i,VALUE=0";
const char fmt_gattsetchar[]    PROGMEM = "AT+GATTCHAR=%i,%s";
const char fmt_gattgetchar[]    PROGMEM = "AT+GATTCHAR=%i";

void Adafruit_BluefruitLE_GATT::error(const __FlashStringHelper*err) {
  Serial.print("### ");
  Serial.println(err);
  while (1);
}

void Adafruit_BluefruitLE_GATT::setGattDeviceName(const char *name) {
  char cmd[strlen_P(fmt_gapdevname) - 2 + strlen(name) + 1];
  sprintf_P(cmd, fmt_gapdevname, name);
  if (! sendCommandCheckOK(cmd) ) {
    error(F("Could not set device name?"));
  }
}

int8_t Adafruit_BluefruitLE_GATT::addGattService(const char *uuid128) {
  char cmd[strlen_P(fmt_gattaddservice) - 2 + strlen(uuid128) + 1];
  sprintf_P(cmd, fmt_gattaddservice, uuid128);
  int32_t pos;
  if (! sendCommandWithIntReply(cmd, &pos)) {
    error(F("Could not add service"));
  }
  return (int8_t) pos;
}

int8_t Adafruit_BluefruitLE_GATT::addGattCharacteristic(uint16_t uuid16, CharacteristicProperties props, byte minLen, byte maxLen) {
  char cmd[strlen_P(fmt_gattaddchar) + 2 + 1 + 1 + 1];
  sprintf_P(cmd, fmt_gattaddchar, uuid16, props, (uint16_t) minLen, (uint16_t) maxLen);
  int32_t pos;
  if (! sendCommandWithIntReply(cmd, &pos)) {
    error(F("Could not add characteristic"));
  }
  return (int8_t) pos;
}

void Adafruit_BluefruitLE_GATT::setGattCharacteristicValue(const int8_t id, byte *value, uint16_t len) {
  if (len == 0) {
    error(F("Characteristic value length cannot be 0"));
  }
  // AT+GATTCHAR takes each byte in hex separated by a dash, e.g. 4 bytes: xx-xx-xx-xx
  char str[len*3 - 1 + 1];
  for (uint16_t i=0; i<len; i++) {
    sprintf(&str[i*3], "%02X-", value[i]);
  }
  str[len*3 - 1] = '\0';
  
  char cmd[__strlen_P(fmt_gattsetchar) + (len*3 - 1) + 1];
  if (id == 0) { 
    error(F("Characteristic id cannot be 0"));
  }
  // Serial.print("id=");Serial.println(id);
  sprintf_P(cmd, fmt_gattsetchar, id, str);
  if (! sendCommandCheckOK(cmd)) {
    error(F("Could not set characteristic value"));
  }
}

uint16_t Adafruit_BluefruitLE_GATT::getGattCharacteristicValue(const int8_t id, byte *reply, uint16_t maxLen) {
  char cmd[strlen_P(fmt_gattgetchar) + 2 + 1];
  sprintf_P(cmd, fmt_gattgetchar, id);
  
  // AT+GATTCHAR returns each byte in hex separated by a dash, e.g. 4 bytes: xx-xx-xx-xx
  char replyStr[maxLen*3];  // includes terminating '\0'
  uint16_t strLen;
  if (! sendCommandWithStringReply(cmd, replyStr, &strLen)) {
    error(F("Could not get characteristic value"));
  }
  
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


