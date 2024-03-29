/*  
 *   Transmitter side Code
 * 
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

void setup() {
  
  Serial.begin(9600);
//  (int ss, int reset, int dio0)
  LoRa.setPins(10, 8, 9);
  while (!Serial);  
  Serial.println("LoRa Sender");
  int a =  LoRa.begin(433E6);
  Serial.println(a);
  if (!LoRa.begin(433E6)) { // or 915E6, the MHz speed of your module
    Serial.println("Starting LoRa failed!");
    while (1);
  }
}

void loop() {
String MyMessage = "Hello World, this is Electronic Clinic";
  LoRa.beginPacket();  
  Serial.println(1);
  LoRa.print(MyMessage);
  LoRa.endPacket();
//  Serial.println(a);
  delay(100);

 }
