## Low Power humidity and temperture monitor

This code is used for a temperature monitor. A LowPower Labs Moteino is used to
sample the HTU21D sensor and transmit the data. A voltage divider is setup to sample
the VIN voltage using the band gap as a reference. 
On startup the sensor is sampled every 10 seconds for an hour. After an hour the
sampling is once a minute. 

Set NODE_ID to unique ID for each node.
    
Tranmitted data:  
[402] H:59.12 T:22.36 B:9350   [RX_RSSI:-34]\r\n  

Where
- [402] is the node ID
- H humidity 
- T Temperature C
- B Battery level in mV.
- RX_RSSI node received ACK signal strength from gateway.`


