## 433Mhz bridge for humitidy/temperature senors for ESPHome.
The ESPHome folder contains the yaml file to generate the ESP8266 code to read the RX serial data from the lowpower GW and send the WIFI update packets to ESPHome. Code is generated and downloaded using esphome.

The lowpowerGW folder contains the code to commicate with the remote sensors over 433Mhz RF. 

The SHT21_RF contains code for the remote humitity/temperature sensors. Remote node ID must be unique for eash sensor. Adding sensors requires adding there node ID to the ESPHome yaml file.


The RF433WIFI folder contains a Kicad schematic for sample hardware.
