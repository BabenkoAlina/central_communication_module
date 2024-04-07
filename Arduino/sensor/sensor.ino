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

#define RECEIVER_ADDRESS 0x01

#define IS_PAIRED_ADDRESS 0
#define SERVER_ADDRESS_ADDRESS 1
#define SECRET_HASH_ADDRESS 3
#define notPaired 32

uint16_t serverAddress;
byte secretHashBytes[32];
byte isPaired;

int gpioPin = 1;

struct Packet {
    uint8_t magicByte;
    uint8_t header;
    uint16_t addressTo;
    uint16_t addressFrom;
    char message[58];
};

void createPacket(Packet& packet, uint8_t header, uint16_t addressTo, uint16_t addressFrom, const char* message) {
    packet.magicByte = 50; // Example magic byte
    packet.header = header;
    packet.addressTo = addressTo;
    packet.addressFrom = addressFrom;
    strncpy(packet.message, message, 57);
    packet.message[57] = '\0'; // Ensure null-terminated string
}

void pairing() {
    uint16_t serverAddress = 0xABCD; // Example server address
    byte secretHash[32];
    for (int i = 0; i < 32; i++) {
        secretHash[i] = (byte)i; // Just an example hash
    }

    EEPROM.write(SERVER_ADDRESS_ADDRESS, serverAddress >> 8);
    EEPROM.write(SERVER_ADDRESS_ADDRESS + 1, serverAddress & 0xFF);

    for (int i = 0; i < 32; i++) {
        EEPROM.write(SECRET_HASH_ADDRESS + i, secretHash[i]);
    }

    EEPROM.write(IS_PAIRED_ADDRESS, 0xFF); // 0xFF means paired

    Serial.println("Device paired successfully.");

    Packet packet;
    createPacket(packet, 1, serverAddress, 0x00, "Pairing successful");
    LoRa.beginPacket();
    LoRa.write((uint8_t*)&packet, sizeof(Packet));
    LoRa.endPacket();
}

void setup() {
    Serial.begin(9600);
    LoRa.setPins(10, 8, 9);
    while (!Serial);
    Serial.println("LoRa Receiver");
    if (!LoRa.begin(433E6)) {
        Serial.println("Starting LoRa failed!");
        while (1);
    }

    isPaired = EEPROM.read(IS_PAIRED_ADDRESS);
    if (isPaired == notPaired) {
        Serial.println("I am ready for pairing!");
        pairing();
    } else {
        Serial.println("Already paired. Sending message.");
    }
}

void loop() {
    if (isPaired != notPaired) {
        Packet packet;
        createPacket(packet, 255, serverAddress, 0x00, "Hello World, this is Electronic Clinic");
        LoRa.beginPacket();
        LoRa.write((uint8_t*)&packet, sizeof(Packet));
        LoRa.endPacket();
        delay(1000); // Send message every second
    }
}

}
