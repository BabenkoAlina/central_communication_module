
/*
 *  HUB
 *  For ESP32-WROOM-32
 *
  Module SX1278
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

#include <Curve25519.h>
#include <SHA3.h>
#include <AES.h>
#include <base64.hpp>

#define SERVER_ADDRESS 0x00

#define HEADER_PAIRING_REQUEST 1
#define HEADER_PAIRING_RESPONSE 2
#define HEADER_PAIRING_PROCEDURE 3
#define HEADER_ENCRYPTED_HELLO 128
#define HEADER_INFORMAL_MESSAGE 160
#define HEADER_MESSAGE 255

struct SensorStruct {
    char type[50];
    char measurement[50];
    uint8_t hashedKey[32];
};

unsigned char base64_buffer[256];
std::map<uint16_t, SensorStruct> dataSPIFFS;

uint8_t privateKeyHub[32];
uint8_t publicKeyHub[32];
uint8_t combinedKey[32];
uint8_t hashedKey[32];


AES256 cipherBlock;
SHA3_256 sha3;




struct Packet {
    uint8_t magicByte;
    uint8_t header;
    uint16_t addressTo;
    uint16_t addressFrom;
    uint8_t message[58];
};

void createPacket(Packet& packet, uint8_t header, uint16_t addressTo, uint16_t addressFrom, const uint8_t* message) {
    packet.magicByte = 50;
    packet.header = header;
    packet.addressTo = addressTo;
    packet.addressFrom = addressFrom;
    memcpy(packet.message, message, 58);
}

void parsePacket(const uint8_t* buffer, Packet& packet) {
    memcpy(&packet, buffer, sizeof(Packet));
}


void setup() {
    Serial.begin(9600);
//  (int ss, int reset, int dio0)
    LoRa.setPins(2, 32, 33);
    // Enable LoRa CRC
    
    while (!Serial);
    Serial.println("ESP32 Hub");
    if (!LoRa.begin(433E6)) {
        Serial.println("Starting LoRa failed!");
        while (1);
    }
    LoRa.enableCrc();
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
        for (int i = 0; i < sizeof(receivedPacket.message); ++i) {
            Serial.print((char)receivedPacket.message[i]);
        }
        Serial.println();
    }
}
