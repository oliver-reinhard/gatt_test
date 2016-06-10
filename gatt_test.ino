#include <Arduino.h>
#include <stdio.h>
#include <SPI.h>
//#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_GATT.h"

#define CONTROL_CYCLE_DURATION         5000L // [ms]

#define BUFSIZE                        128   // Size of the read buffer for incoming data
#define BLE_VERBOSE_MODE               true  // If set to 'true' enables debug output

// SHARED SPI SETTINGS
// ----------------------------------------------------------------------------------------------
// Declare the pins to use for HW and SW SPI communication.
// SCK, MISO and MOSI should be connected to the HW SPI pins on the Uno when
// using HW SPI.  This should be used with nRF51822 based Bluefruit LE modules
// that use SPI (Bluefruit LE SPI Friend).
// ----------------------------------------------------------------------------------------------
#define BLUEFRUIT_SPI_CS               8
#define BLUEFRUIT_SPI_IRQ              7
#define BLUEFRUIT_SPI_RST              4    // Optional but recommended, set to -1 if unused


/*
 * Create the bluefruit object using hardware SPI (SCK/MOSI/MISO hardware SPI pins and then user selected CS/IRQ/RST)
 */
Adafruit_BluefruitLE_GATT ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

/* The service information */
int8_t controllerServiceId;
int8_t waterTempMeasureCharId;
int8_t ambientTempMeasureCharId;
int8_t targetTempCharId;
int8_t tankCapacityCharId;


void setup(void) {
    
  while (!Serial); // required for Flora & Micro
  delay(500);

  Serial.begin(115200);

  randomSeed(micros());


  if ( !ble.begin(BLE_VERBOSE_MODE))  {
    ble.error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }

  /* Perform a factory reset to make sure everything is in a known state */
  if (! ble.factoryReset()) {
     ble.error(F("Couldn't factory reset"));
  }

  /* Disable command echo from Bluefruit */
  ble.echo(false);

  /* Print Bluefruit information */
  ble.info();

  // this line is particularly required for Flora, but is a good idea
  // anyways for the super long lines ahead!
  // ble.setInterCharWriteDelay(5); // 5 ms

  ble.setGattDeviceName("Boiler Controller");
  controllerServiceId =      ble.addGattService("4C-EF-DD-58-CB-95-44-50-90-FB-F4-04-DC-20-2F-7C");
  waterTempMeasureCharId =   ble.addGattCharacteristic(0x0001, CHAR_PROP_NOTIFY, 2, 2);
  ambientTempMeasureCharId = ble.addGattCharacteristic(0x0002, CHAR_PROP_READ,   2, 4);
  targetTempCharId =         ble.addGattCharacteristic(0x0003, CHAR_PROP_READ | CHAR_PROP_WRITE, 2, 4);
  tankCapacityCharId =       ble.addGattCharacteristic(0x0004, CHAR_PROP_READ | CHAR_PROP_WRITE, 4, 4);

  /* Add the Heart Rate Service to the advertising data (needed for Nordic apps to detect the service) */
  ble.sendCommandCheckOK( F("AT+GAPSETADVDATA=02-01-06-05-02-0d-18-0a-18") );

  /* Reset the device for the new service setting changes to take effect */
  ble.reset();
}

void loop(void) {
  
  static unsigned long controlCycleStart = 0L;
  
  unsigned long now = millis();
  unsigned long elapsed = now - controlCycleStart;
  
  if (controlCycleStart == 0L || elapsed >= CONTROL_CYCLE_DURATION) {
    controlCycleStart = now;
    elapsed = 0L;
  }

  if (elapsed == 0L) {
    byte bytes[sizeof(int)];
    
    int waterTemp = random(20, 42); 
    memcpy(bytes, &waterTemp, sizeof(int));
    reverseBytes(bytes, sizeof(int));
  //Serial.print("**waterTempMeasureCharId="); Serial.println(waterTempMeasureCharId);
    ble.setGattCharacteristicValue(waterTempMeasureCharId, bytes, sizeof(int));
    
    long ambientTemp = random(13, 28);
    memcpy(bytes, &ambientTemp, sizeof(long));
    reverseBytes(bytes, sizeof(long));
    ble.setGattCharacteristicValue(ambientTempMeasureCharId, bytes, sizeof(long));

    Serial.print(F("New water temp = 0x"));
    Serial.print(waterTemp, HEX);
    Serial.print(F(", ambient temp = 0x"));
    Serial.println(ambientTemp, HEX);
  }

  static long previousTargetTemp = 0;
  byte rawValue[4];
  ble.getGattCharacteristicValue(targetTempCharId, rawValue, sizeof(rawValue));
  reverseBytes(rawValue, sizeof(rawValue));
  long targetTemp;  
  memcpy(&targetTemp, rawValue, sizeof(rawValue));
  if(targetTemp != previousTargetTemp) {
    previousTargetTemp = targetTemp;
    Serial.print(F("** New target temp = 0x"));
    Serial.println(targetTemp, HEX);
  }

  static float previousTankCapacity = 0.0;
  //byte rawValue[4];
  ble.getGattCharacteristicValue(tankCapacityCharId, rawValue, sizeof(rawValue));
  reverseBytes(rawValue, sizeof(rawValue));
  float tankCapacity;  
  memcpy(&tankCapacity, rawValue, sizeof(rawValue));
  if(tankCapacity != previousTankCapacity) {
    previousTankCapacity = tankCapacity;
    Serial.print(F("** New tank capacity = "));
    Serial.println(tankCapacity);
  }


  /* Delay before next measurement update */
  delay(1000);
}

void reverseBytes(byte *buf, uint16_t len) {
  for(uint16_t i=0; i<len/2; i++) {
    byte b = buf[i];
    buf[i] = buf[len-1-i];
    buf[len-1-i] = b;
  }
}

