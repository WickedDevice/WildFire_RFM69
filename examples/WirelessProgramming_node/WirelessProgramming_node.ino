/*
 * Copyright (c) 2013 by Felix Rusu <felix@lowpowerlab.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */

// This sketch is an example of how wireless programming can be achieved with a Moteino
// that was loaded with a custom 1k Optiboot that is capable of loading a new sketch from
// an external SPI flash chip
// The sketch includes logic to receive the new sketch 'over-the-air' and store it in
// the FLASH chip, then restart the Moteino so the bootloader can continue the job of
// actually reflashing the internal flash memory from the external FLASH memory chip flash image
// The handshake protocol that receives the sketch wirelessly by means of the RFM69 radio
// is handled by the SPIFLash/WirelessHEX69 library, which also relies on the RFM69 library
// These libraries and custom 1k Optiboot bootloader are at: http://github.com/lowpowerlab

#include <WildFire_RFM69.h>
#include <SPI.h>
#include <WildFire_SPIFlash.h>
#include <avr/wdt.h>
#include <WildFire_WirelessHEX69.h>
#include <WildFire.h>
WildFire wf;

#define MYID        55       // node ID used for this unit
#define NETWORKID   250
//Match frequency to the hardware version of the radio on your Moteino (uncomment one):
#define FREQUENCY   RF69_433MHZ
//#define FREQUENCY   RF69_868MHZ
//#define FREQUENCY     RF69_915MHZ
#define IS_RFM69HW  //uncomment only for RFM69HW! Leave out if you have RFM69W!
#define SERIAL_BAUD 115200
#define ACK_TIME    50  // # of ms to wait for an ack
#define ENCRYPTKEY  "sampleEncryptKey"
#define LED         6
#define BLINKPERIOD 200

WildFire_RFM69 radio;
char input = 0;
long lastPeriod = -1;

WildFire_SPIFlash flash;

void setup(){
  wf.begin();
  pinMode(LED, OUTPUT);
  Serial.begin(SERIAL_BAUD);
  radio.initialize(FREQUENCY,MYID,NETWORKID);
  radio.encrypt(ENCRYPTKEY); //OPTIONAL
#ifdef IS_RFM69HW
  radio.setHighPower(); //only for RFM69HW!
#endif
  Serial.print("Start node...");

  if (flash.initialize())
    Serial.println("SPI Flash Init OK!");
  else
    Serial.println("SPI Flash Init FAIL!");
}

void loop(){
  // This part is optional, useful for some debugging.
  // Handle serial input (to allow basic DEBUGGING of FLASH chip)
  // ie: display first 256 bytes in FLASH, erase chip, write bytes at first 10 positions, etc
  if (Serial.available() > 0) {
    input = Serial.read();
    if (input == 'd') //d=dump first page
    {
      Serial.println("Flash content:");
      int counter = 0;

      while(counter<=256){
        Serial.print(flash.readByte(counter++), HEX);
        Serial.print('.');
      }
      
      Serial.println();
    }
    else if (input == 'e')
    {
      Serial.print("Erasing Flash chip ... ");
      flash.chipErase();
      while(flash.busy());
      Serial.println("DONE");
    }
    else if (input == 'i')
    {
      Serial.print("DeviceID: ");
      Serial.println(flash.readDeviceId(), HEX);
    }
    else if (input == 'r')
    {
      Serial.print("Rebooting");
      resetUsingWatchdog(true);
    }
    else if (input == 'R')
    {
      Serial.print("RFM69 registers:");
      radio.readAllRegs();
    }
    else if (input >= 48 && input <= 57) //0-9
    {
      Serial.print("\nWriteByte("); Serial.print(input); Serial.print(")");
      flash.writeByte(input-48, millis()%2 ? 0xaa : 0xbb);
    }
  }
  
  // Check for existing RF data, potentially for a new sketch wireless upload
  // For this to work this check has to be done often enough to be
  // picked up when a GATEWAY is trying hard to reach this node for a new sketch wireless upload
  if (radio.receiveDone())
  {
    Serial.print("Got [");
    Serial.print(radio.SENDERID);
    Serial.print(':');
    Serial.print(radio.DATALEN);
    Serial.print("] > ");
    for (byte i = 0; i < radio.DATALEN; i++)
      Serial.print((char)radio.DATA[i], HEX);
    Serial.println();
    CheckForWirelessHEX(radio, flash, true);
    Serial.println();
  }
  //else Serial.print('.');
  
  ////////////////////////////////////////////////////////////////////////////////////////////
  // Real sketch code here, let's blink the onboard LED
  if ((int)(millis()/BLINKPERIOD) > lastPeriod)
  {
    lastPeriod++;
    digitalWrite(LED, lastPeriod%2);
  }
  ////////////////////////////////////////////////////////////////////////////////////////////
}
