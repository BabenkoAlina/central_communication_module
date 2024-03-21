/*
 *  HUB 
 *  Receiver Side Code
 *  For ESP32-WROOM-32
 * 
  Module SX1278 // Arduino UNO/NANO    
    Vcc         ->   3.3V
    MISO        ->   D12
    MOSI        ->   D11     
    SLCK        ->   D13
    NSS         ->   D10
    GND         ->   GND
 */

#include <SPI.h>
#include <LoRa.h>  
String inString = "";
String MyMessage = "";

void setup() {
  Serial.begin(9600);
  LoRa.setPins(10, 8, 9);
  while (!Serial);
  Serial.println("LoRa Receiver");
  if (!LoRa.begin(433E6)) 
      Serial.println("Starting LoRa failed!");
    while (1);
  }
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) { 
    while (LoRa.available())
    {
      int inChar = LoRa.read();
      inString += (char)inChar;
      MyMessage = inString;       
    }
    inString = "";     
    LoRa.packetRssi();    
    Serial.println(MyMessage);  
  }  
}