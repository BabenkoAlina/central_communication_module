
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

//int a = LoRa.enableCrc();
#include <SPI.h>
#include <LoRa.h>
#include "FS.h"
#include "SPIFFS.h"
#include <map>
#include <string>
#include <sstream>

struct Packet {
    uint8_t magicByte;
    uint8_t header;
    uint16_t addressTo;
    uint16_t addressFrom;
    char message[58];
};

std::map<uint16_t, std::string> dataSPIFFS; // Use address as key

void parsePacket(const uint8_t* buffer, Packet& packet) {
    memcpy(&packet, buffer, sizeof(Packet));
}

void handlePairing(Packet& packet) {
    Serial.print("Received pairing message from ");
    Serial.print(packet.addressFrom);
    Serial.println(": Pairing successful");

    // Assuming dataSPIFFS is a map of address to hashed key
    std::string hashedKey(packet.message);
    dataSPIFFS[packet.addressFrom] = hashedKey;

    Serial.println("Sensor added to SPIFFS.");
}

void setup() {
    Serial.begin(9600);
    LoRa.setPins(2, 32, 33);
    while (!Serial);
    Serial.println("ESP32 Hub");
    if (!LoRa.begin(433E6)) {
        Serial.println("Starting LoRa failed!");
        while (1);
    }

    if (!SPIFFS.begin(true)) {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }
}

void loop() {
    int packetSize = LoRa.parsePacket();
    if (packetSize == sizeof(Packet)) {
        Packet receivedPacket;
        uint8_t buffer[sizeof(Packet)];
        for (int i = 0; i < sizeof(Packet); ++i) {
            buffer[i] = LoRa.read();
        }
        parsePacket(buffer, receivedPacket);
        if (receivedPacket.magicByte == 50) { // Check if it's our protocol
            if (receivedPacket.header == 1) { // Pairing header
                handlePairing(receivedPacket);
            } else {
                Serial.print("Received message: ");
                Serial.println(receivedPacket.message);
            }
        }
    }
}
