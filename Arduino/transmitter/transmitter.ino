/* 
 *  Sensor Monitoring using Arduino and LoRa SX1278 Module, transmitter code
  Module SX1278 // Arduino UNO/NANO    
    Vcc         ->   3.3V
    MISO        ->   D12
    MOSI        ->   D11     
    SLCK        ->   D13
    Nss         ->   D10
    GND         ->   GND
 */
#include <SPI.h>
#include <LoRa.h>
int pot = A2;

void setup() {
  Serial.begin(9600);
  pinMode(pot,INPUT);
  
  while (!Serial);  
  Serial.println("LoRa Sender");
  if (!LoRa.begin(433E6)) { // or 915E6, the MHz speed of your module
    Serial.println("Starting LoRa failed!");
    while (1);
  }
}

void loop() {
  int val = map(analogRead(pot),0,1024,0,255);
  LoRa.beginPacket();  
  LoRa.print(val);
  LoRa.endPacket();
  delay(50);
 
}
