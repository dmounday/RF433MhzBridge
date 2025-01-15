#include "Arduino.h"
// **********************************************************************************
// This code is used for a temperature monitor. A LowPowser Labs Moteino is used to
// sample the HTU21D sensor and transmit the data. A voltage divider is setup to sample
// the VIN voltage using the band gap as a reference. 
// On startup the sensor is sampled every 10 seconds for an hour. After an hour the
// sampling is once a minute. 
// SET NODE_ID to unique ID for each node.
// 
//
//***********************************************************************************
// The RFM code is Copyright Felix Rusu 2020, http://www.LowPowerLab.com/contact
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
// Everything is put to sleep including the MCU, the radio (if any) and the FlashMem chip
//**** SETTINGS *********************************************************************************************
// #define WITH_SPIFLASH           //comment this line out if you don't have the FLASH-MEM chip on your Moteino
//*********************************************************************************************
#include <LowPower.h>   //https://www.github.com/lowpowerlab/lowpower
#include <RFM69.h>      //https://www.github.com/lowpowerlab/rfm69
#include <RFM69_ATC.h>  //included in RFM69
/*
 HTU21D Humidity Sensor Example Code
 By: Nathan Seidle
 SparkFun Electronics
 Date: September 15th, 2013
 License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).

 Uses the HTU21D library to display the current humidity and temperature

 Open serial monitor at 115200 baud to see readings. Errors 998 if not sensor is detected. Error 999 if CRC is bad.

 Hardware Connections (Breakoutboard to Arduino):
 -VCC = 3.3V
 -GND = GND
 -SDA = A4 (use inline 10k resistor if your board is 5V)
 -SCL = A5 (use inline 10k resistor if your board is 5V)

 */

#include "Adafruit_HTU21DF.h"
// #define LED_BUILTIN 9   // moteino is D9
// ******************************************************************************************
// Set NODEID to unique id.
const uint16_t NODEID {402};        // MUST BE SET TO UNIQUE VALUE FOR EACH NODE !!!!!!!!!!!!!!
const uint16_t NETWORKID {200};
const uint8_t GATEWAYID {2};        // Node ID of RF to ESPHome gateway. Should typically be 1.
const uint8_t FREQUENCY {RF69_433MHZ};  // RF69_915MHZ  //match the RFM69 version! Others: RF69_433MHZ, RF69_868MHZ
// #define FREQUENCY_EXACT 916000000
#define ENCRYPTKEY "sampleEncryptKey"  // same 16 characters on all nodes, comment this line to disable encryption
#define IS_RFM69HW_HCW                 // uncomment only for RFM69HW/HCW! Leave out if you have RFM69W/CW!
//*********************************************************************************************
#define ENABLE_ATC  // comment out this line to disable AUTO TRANSMISSION CONTROL
#define ATC_RSSI -68
//*********************************************************************************************
#define SERIAL_BAUD 115200  // comment out to turn off serial output
#ifdef SERIAL_BAUD
#define DEBUG(input) Serial.print(input)
#define DEBUGln(input) Serial.println(input)
#define DEBUGHEX(x) Serial.print(x, HEX)
#define DEBUGDEC(x) Serial.print(x, DEC)
#define DEBUGflush() Serial.flush()
#else
#define DEBUG(input)
#define DEBUGln(input)
#define DEBUGflush()
#define DEBUGHEX(x)
#define DEBUGDEC(x)
#endif

#define LED_HIGH digitalWrite(LED_BUILTIN, HIGH)
#define LED_LOW digitalWrite(LED_BUILTIN, LOW)
#if defined(WITH_SPIFLASH)
#include <SPI.h>                      //comes with Arduino IDE (www.arduino.cc)
#include <SPIFlash.h>                 //get it here: https://www.github.com/lowpowerlab/spiflash// Battery monitor thru voltage divider.
SPIFlash flash(SS_FLASHMEM, 0xEF30);  // EF30 for 4mbit  Windbond chip (W25X40CL)
#endif
//***********************************************************************************************
Adafruit_HTU21DF htu = Adafruit_HTU21DF();
float htu_temp;
float htu_rel_hum;
//*********************************************************************************************
#ifdef ENABLE_ATC
RFM69_ATC radio;
#else
RFM69 radio;
#endif

char buff[61];  // max packet size is 61 with encryption enabled
char Fstr[10];
byte buffLen;
//*********************************************************************************************
const uint16_t FAST_XMIT {10000};  // read and send data every this many ms (WDT sleep)
const uint16_t SLOW_XMIT {60000};  // once an hour.
//*********************************************************************************************
const int hourSamples {3600/10}; // samples per hour 
int sampleCnt {0};
//**********************************************************************************************

void setup() {
  if (!htu.begin()) {
    sprintf(buff, "HTU sensor not found.");
    DEBUGln(buff);
  }
#ifdef WITH_SPIFLASH
  if (flash.initialize())
    flash.sleep();
#endif
#ifdef SERIAL_BAUD
  Serial.begin(SERIAL_BAUD);
#endif
  pinMode(LED_BUILTIN, OUTPUT);

  radio.initialize(FREQUENCY, NODEID, NETWORKID);
#ifdef IS_RFM69HW_HCW
  radio.setHighPower();  // uncomment only for RFM69HW!
#endif
#ifdef FREQUENCY_EXACT
  radio.setFrequency(FREQUENCY_EXACT);  // set frequency to some custom frequency
#endif
#ifdef ENCRYPTKEY
  radio.encrypt(ENCRYPTKEY);
#endif

#ifdef ENABLE_ATC
  radio.enableAutoPower(ATC_RSSI);
#endif
  radio.sleep();
  sprintf(buff, "NodeID: %d", NODEID);
  DEBUG(buff);
  sprintf(buff, "  Transmitting at %d Mhz...", FREQUENCY == RF69_433MHZ ? 433 : FREQUENCY == RF69_868MHZ ? 868                                                                                                       : 915);
  DEBUGln(buff);
}

uint16_t readAvgVcc(uint8_t);

void loop() {
  htu_temp = htu.readTemperature();
  htu_rel_hum = htu.readHumidity();
  dtostrf(htu_rel_hum, 3, 2, Fstr);
  sprintf(buff, "H:%s", Fstr);
  buffLen = strlen(buff);
  dtostrf(htu_temp, 3, 2, Fstr);
  sprintf(buff+buffLen, " T:%s", Fstr);
  buffLen = strlen(buff);
  sprintf(buff+buffLen, " B:%d", readAvgVcc(0));
  buffLen = strlen(buff);

  LED_HIGH;
  DEBUG(buff);
  DEBUG(" .. ");
  if (radio.sendWithRetry(GATEWAYID, buff, buffLen, 2, 40)) {
    DEBUG("ok!");
  } else {
    DEBUG("nok...");
  }
  DEBUGln();
  radio.sleep();
  LED_LOW;
  DEBUGflush();
  if ( sampleCnt > hourSamples )
    LowPower.longPowerDown( SLOW_XMIT);
  else {
    ++sampleCnt;
    LowPower.longPowerDown( FAST_XMIT );
  }
}
