/*  
 *  Sensor
 *  Transmitter side Code
 *  For Arduino Nano
 * 
  Module SX1278 // Arduino UNO/NANO    
    Vcc         ->   3.3V
    MISO        ->   D19
    MOSI        ->   D23     
    SLCK        ->   D18
    NSS         ->   D2
    GND         ->   GND
 */

#include <SPI.h>
#include <LoRa.h>

void setup() {
  Serial.begin(9600);
//  (int ss, int reset, int dio0)
  LoRa.setPins(2, 34, 35);
  while (!Serial);  
  Serial.println("LoRa Sender");
  int a =  LoRa.begin(433E6);
  Serial.println(a);
  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
}

void loop() {
String MyMessage = "Hello World, this is Electronic Clinic";
  LoRa.beginPacket();  
  LoRa.print(MyMessage);
  LoRa.endPacket();
  delay(100);
 }