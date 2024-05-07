/*  
 *  Sensor
 *  For Arduino Nano
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
#include <EEPROM.h>

#include <Curve25519.h>
#include <SHA3.h>
#include <AES.h>
#include <base64.hpp>

#define SENSOR_ADDRESS 0x01

#define IS_PAIRED_ADDRESS 0
#define SERVER_ADDRESS 1
#define SECRET_HASH_ADDRESS 3

#define notPaired 32
#define Paired 64

#define HEADER_PAIRING_REQUEST 1
#define HEADER_PAIRING_RESPONSE 2
#define HEADER_PAIRING_PROCEDURE 3
#define HEADER_ENCRYPTED_HELLO 128
#define HEADER_INFORMAL_MESSAGE 160
#define HEADER_MESSAGE 255


uint16_t serverAddress;
uint8_t secretHashBytes[32];
byte isPaired;

uint8_t privateKeySensor[32];

uint8_t publicKeySensor[32];

uint8_t combinedKey[32];
uint8_t hashedKey[32];

AES256 cipherBlock;
SHA3_256 sha3;

int gpioPin = 0;

unsigned char base64_buffer[256];

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
    strncpy((char*)packet.message, message, sizeof(packet.message));
}


void parsePacket(const uint8_t* buffer, Packet& packet) {
    memcpy(&packet, buffer, sizeof(Packet));
}

void setup() {
    Serial.begin(9600);
    LoRa.setPins(10, 8, 9);
    // Enable LoRa CRC
    
    while (!Serial);
    Serial.println("LoRa Sensor");
    if (!LoRa.begin(433E6)) {
        Serial.println("Starting LoRa failed!");
        while (1);
    }
    LoRa.enableCrc();
}

void loop() {
    Packet packet;
    uint8_t message[] = "Hello, world!";
    createPacket(packet, HEADER_MESSAGE, serverAddress, SENSOR_ADDRESS, message);
    uint8_t buffer[sizeof(Packet)];

    memcpy(buffer, &packet, sizeof(Packet));

    int a = LoRa.beginPacket();
    Serial.println(a);

    for (int i = 0; i < sizeof(Packet); ++i) {
        LoRa.write(buffer[i]);
    }
    LoRa.endPacket();
    delay(200);
}
