/*
 *  HUB 
 *  For ESP32-WROOM-32
 * 
  Module SX1278 // Arduino UNO/NANO    
    Vcc         ->   3.3V
    MISO        ->   D19
    MOSI        ->   D23     
    SLCK        ->   D18
    Nss         ->   D2
    GND         ->   GND
 */

#include <SPI.h>
#include <LoRa.h>  
String inString = "";
String MyMessage = "";

void setup() {
  Serial.begin(9600);
//  (int ss, int reset, int dio0)
  LoRa.setPins(2, 32, 33);
  while (!Serial);
  Serial.println("LoRa Receiver");
  if (!LoRa.begin(433E6)) {
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
