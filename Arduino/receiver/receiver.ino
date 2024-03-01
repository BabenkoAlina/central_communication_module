/* 
 *  Receiver Side Code
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

#define RECEIVER_ADDRESS 0x01 // Address of this receiver

String inString = "";    // string to hold incoming characters

void setup() {
  Serial.begin(9600);
  LoRa.setPins(53, 48, 49);
  while (!Serial);
  Serial.println("LoRa Receiver");
  if (!LoRa.begin(433E6)) { // or 915E6
    Serial.println("Starting LoRa failed!");
    while (1);
  }
}

void loop() {
  // try to parse packet
  int packetSize = LoRa.parsePacket();

  if (packetSize) {
    // read packet    
    byte senderAddress = LoRa.read(); // Read the sender address

    // If the packet is not intended for this receiver, ignore it
    if (senderAddress != RECEIVER_ADDRESS) {
      // Clear the buffer
      while (LoRa.available())
        LoRa.read();
      return; // Exit the loop
    }

    // Read the message from the packet
    while (LoRa.available()) {
      int inChar = LoRa.read();
      inString += (char)inChar;
    }

    // Print the received message
    Serial.println(inString);
    
    // Clear the buffer and reset for next packet
    inString = "";
  }
}
