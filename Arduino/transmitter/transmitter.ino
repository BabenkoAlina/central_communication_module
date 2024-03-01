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

#define RECEIVER_1_ADDRESS 0x01 // Address of Receiver 1
#define RECEIVER_2_ADDRESS 0x02 // Address of Receiver 2

void setup() {
  Serial.begin(9600);
  LoRa.setPins(53, 48, 49);
  while (!Serial);  
  Serial.println("LoRa Sender");
  if (!LoRa.begin(433E6)) { // or 915E6, the MHz speed of your module
    Serial.println("Starting LoRa failed!");
    while (1);
  }
}

void loop() {
  // Define the message and the receiver address
  String MyMessage = "Hello World, this is Electronic Clinic";
  byte receiverAddress = RECEIVER_1_ADDRESS; // Address the message is intended for

  // Begin the LoRa packet transmission
  LoRa.beginPacket();
  
  // Send the receiver address as the first byte of the packet
  LoRa.write(receiverAddress);

  // Send the message
  LoRa.print(MyMessage);

  // End the LoRa packet transmission
  LoRa.endPacket();
  
  delay(100); // Delay between transmissions
}
