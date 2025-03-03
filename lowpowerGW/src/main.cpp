// **********************************************************************************
//            !!!!     ATTENTION:    !!!!
// This is just a simple receiving sketch that will work with most examples
// in the RFM69 library.
//
// If you're looking for the Gateway sketch to use with your RaspberryPi,
// as part of the PiGateway software interface (lowpowerlab.com/gateway),
// this is the wrong sketch. Use this sketch instead: PiGateway:
// https://github.com/LowPowerLab/RFM69/blob/master/Examples/PiGateway/PiGateway.ino
// **********************************************************************************
// Sample RFM69 receiver/gateway sketch, with ACK and optional encryption, and Automatic Transmission Control
// Passes through any wireless received messages to the serial port & responds to ACKs
// It also looks for an onboard FLASH chip, if present
// **********************************************************************************
// Copyright Felix Rusu 2016, http://www.LowPowerLab.com/contact
// **********************************************************************************
// License
// **********************************************************************************
// This program is free software; you can redistribute it 
// and/or modify it under the terms of the GNU General    
// Public License as published by the Free Software       
// Foundation; either version 3 of the License, or        
// (at your option) any later version.                    
//                                                        
// This program is distributed in the hope that it will   
// be useful, but WITHOUT ANY WARRANTY; without even the  
// implied warranty of MERCHANTABILITY or FITNESS FOR A   
// PARTICULAR PURPOSE. See the GNU General Public        
// License for more details.                              
//                                                        
// Licence can be viewed at                               
// http://www.gnu.org/licenses/gpl-3.0.txt
//
// Please maintain this license information along with authorship
// and copyright notices in any redistribution of this code
// **********************************************************************************
#include <RFM69.h>         //get it here: https://www.github.com/lowpowerlab/rfm69
#include <RFM69_ATC.h>     //get it here: https://www.github.com/lowpowerlab/rfm69
#ifdef FLASHMEM
#include <SPIFlash.h>      //get it here: https://www.github.com/lowpowerlab/spiflash
#define SS_FLASHMEM 8
SPIFlash flash(SS_FLASHMEM, 0xEF30); //EF30 for 4mbit  Windbond chip (W25X40CL)
#endif
//*********************************************************************************************
//************ IMPORTANT SETTINGS - YOU MUST CHANGE/CONFIGURE TO FIT YOUR HARDWARE *************
//*********************************************************************************************
#define NODEID        2    // using 2 to avoid older network for pool. Should typically be 1 for a Gateway
#define NETWORKID     200  //the same on all nodes that talk to each other
//Match frequency to the hardware version of the radio on your Moteino (uncomment one):
#define FREQUENCY     RF69_433MHZ
//#define FREQUENCY     RF69_868MHZ
//#define FREQUENCY     RF69_915MHZ
#define ENCRYPTKEY    "sampleEncryptKey" //exactly the same 16 characters/bytes on all nodes!
#define IS_RFM69HW_HCW  //uncomment only for RFM69HW/HCW! Leave out if you have RFM69W/CW!
//*********************************************************************************************
//Auto Transmission Control - dials down transmit power to save battery
//Usually you do not need to always transmit at max output power
//By reducing TX power even a little you save a significant amount of battery power
//This setting enables this gateway to work with remote nodes that have ATC enabled to
//dial their power down to only the required level
#define ENABLE_ATC    //comment out this line to disable AUTO TRANSMISSION CONTROL
//*********************************************************************************************
#define LED_HIGH digitalWrite(LED_BUILTIN, HIGH)
#define LED_LOW digitalWrite(LED_BUILTIN, LOW)
#define SERIAL_BAUD   9600

#ifdef ENABLE_ATC
  RFM69_ATC radio;
#else
  RFM69 radio;
#endif

bool spy = false; //set to 'true' to sniff all packets on the same network

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(10);
  radio.initialize(FREQUENCY,NODEID,NETWORKID);
#ifdef IS_RFM69HW_HCW
  radio.setHighPower(); //must include this only for RFM69HW/HCW!
#endif
  radio.encrypt(ENCRYPTKEY);
  radio.spyMode(spy);
  //radio.setFrequency(916000000); //set frequency to some custom frequency
  char buff[50];
  sprintf(buff, "\nListening at %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  Serial.println(buff);
#ifdef FLASHMEM
  if (flash.initialize())
  {
    Serial.print("SPI Flash Init OK. Unique MAC = [");
    flash.readUniqueId();
    for (byte i=0;i<8;i++)
    {
      Serial.print(flash.UNIQUEID[i], HEX);
      if (i!=8) Serial.print(':');
    }
    Serial.println(']');
    
    //alternative way to read it:
    //byte* MAC = flash.readUniqueId();
    //for (byte i=0;i<8;i++)
    //{
    //  Serial.print(MAC[i], HEX);
    //  Serial.print(' ');
    //}
  }
  else
    Serial.println("SPI Flash MEM not found (is chip soldered?)...");
#endif 

#ifdef ENABLE_ATC
  Serial.println("RFM69_ATC Enabled (Auto Transmission Control)");
#endif
}

byte ackCount=0;
uint32_t packetCount = 0;
void loop() {
  // process any serial input
  if (Serial.available() > 0) {
    char input = Serial.read();
    if (input == 'r')  // d=dump all register values
      radio.readAllRegs();
    if (input == 'E')  // E=enable encryption
      radio.encrypt(ENCRYPTKEY);
    if (input == 'e')  // e=disable encryption
      radio.encrypt(null);
    if (input == 'p') {
      spy = !spy;
      radio.spyMode(spy);
      Serial.print("SpyMode mode ");
      Serial.println(spy ? "on" : "off");
    }
#ifdef FLASHMEM
    if (input == 'd')  // d=dump flash area
    {
      Serial.println("Flash content:");
      int counter = 0;

      while (counter <= 256) {
        Serial.print(flash.readByte(counter++), HEX);
        Serial.print('.');
      }
      while (flash.busy());
      Serial.println();
    }
    if (input == 'D') {
      Serial.print("Deleting Flash chip ... ");
      flash.chipErase();
      while (flash.busy());
      Serial.println("DONE");
    }
    if (input == 'i') {
      Serial.print("DeviceID: ");
      word jedecid = flash.readDeviceId();
      Serial.println(jedecid, HEX);
    }
#endif
    if (input == 't') {
      byte temperature = radio.readTemperature(-1);  // -1 = user cal factor, adjust for correct ambient
      byte fTemp = 1.8 * temperature + 32;           // 9/5=1.8
      Serial.print("Radio Temp is ");
      Serial.print(temperature);
      Serial.print("C, ");
      Serial.print(fTemp);  // converting to F loses some resolution, obvious when C is on edge between 2 values (ie 26C=78F, 27C=80F)
      Serial.println('F');
    }
  }

  if (radio.receiveDone()) {
    LED_HIGH;
    Serial.print('[');
    Serial.print(radio.SENDERID, DEC);
    Serial.print("] ");
    if (spy) {
      Serial.print("to [");
      Serial.print(radio.TARGETID, DEC);
      Serial.print("] ");
    }
    if (radio.TARGETID == NODEID || spy) {
      for (byte i = 0; i < radio.DATALEN; i++)
        Serial.print((char)radio.DATA[i]);
      Serial.print("   [RX_RSSI:");
      Serial.print(radio.RSSI);
      Serial.print("]");
      if (radio.ACKRequested()) {
        byte theNodeID = radio.SENDERID;
        radio.sendACK();
      }
      Serial.println();
    }
    LED_LOW;
  }
}
