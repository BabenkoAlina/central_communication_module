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

#define SENSOR_ADDRESS 0x01

#define IS_PAIRED_ADDRESS 0
#define SERVER_ADDRESS_ADDRESS 1
#define SECRET_HASH_ADDRESS 3

#define notPaired 32
#define Paired 64

#define HEADER_PAIRING_REQUEST 1
#define HEADER_PAIRING_RESPONSE 2
#define HEADER_PAIRING_PROCEDURE 3
#define HEADER_ENCRYPTED_HELLO 128
#define INFORMAL_MESSAGE 160
#define HEADER_MESSAGE 255


uint16_t serverAddress;
byte secretHashBytes[32];
byte isPaired;

uint8_t privateKeySensor[66];
uint8_t publicKeySensor[132];

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

void parsePacket(const uint8_t* buffer, Packet& packet) {
    memcpy(&packet, buffer, sizeof(Packet));
}



void readEEPROM(){
    serverAddress = EEPROM.read(SERVER_ADDRESS_ADDRESS) << 8 | EEPROM.read(SERVER_ADDRESS_ADDRESS + 1);
    Serial.print("Server Address: ");
    Serial.println(serverAddress);

    Serial.println("Secret Hash:");
    secretHashBytes[32];
    for (int i = 0; i < 32; i++) {
        secretHashBytes[i] = EEPROM.read(SECRET_HASH_ADDRESS + i);
        Serial.print(secretHashBytes[i], HEX);
    }
    Serial.println();
}


void handlePairing() {
    // first step
    // send pairing request

    // придумати де взяти адресу сервера!!!!!!!!!
    Packet packet;
    createPacket(packet, HEADER_PAIRING_REQUEST, serverAddress, SENSOR_ADDRESS, "Requesting pairing");
    LoRa.beginPacket();
    LoRa.write((uint8_t*)&packet, sizeof(Packet));
    LoRa.endPacket();
    delay(2000); // Wait for response 2 seconds


    // second step
    // pair procedure

    // get public key from server
    int packetSize = LoRa.parsePacket();
    if (packetSize == sizeof(Packet)) {
        Packet receivedPacket;
        uint8_t buffer[sizeof(Packet)];
        for (int i = 0; i < sizeof(Packet); ++i) {
            buffer[i] = LoRa.read();
        }
        parsePacket(buffer, receivedPacket);
        // check if my address or broadcast
        if (receivedPacket.addressTo == SENSOR_ADDRESS) {
            if (receivedPacket.magicByte == 50) { // Check if it's our protocol
                if (receivedPacket.header == HEADER_PAIRING_RESPONSE) { // Pairing header
                    receivedPublicKey = receivedPacket.message;
                    Serial.println("Received public key from server");
                    Serial.println(receivedPublicKey);

                    // generate public and private keys
                    // maybe need convert key to string
                    ...

                    // send public key to server
                    Packet responsePacket;
                    createPacket(responsePacket, HEADER_PAIRING_PROCEDURE, serverAddress, SENSOR_ADDRESS,
                                 PublicKeySensor);
                    uint8_t buffer[sizeof(Packet)];
                    memcpy(buffer, &responsePacket, sizeof(Packet));
                    LoRa.beginPacket();
                    for (int i = 0; i < sizeof(Packet); ++i) {
                        LoRa.write(buffer[i]);
                    }
                    LoRa.endPacket();
                    delay(2000); // Wait for response 2 seconds

                    // combine keys and creating hash
                    ...
                }
            }
        }
    }

    // third step
    // receive encrypted hello
    int packetSize = LoRa.parsePacket();
    if (packetSize == sizeof(Packet)) {
        Packet receivedPacket;
        uint8_t buffer[sizeof(Packet)];
        for (int i = 0; i < sizeof(Packet); ++i) {
            buffer[i] = LoRa.read();
        }
        parsePacket(buffer, receivedPacket);
        // maybe need some check
        if (receivedPacket.header == HEADER_ENCRYPTED_HELLO) {
            Serial.print("Received encrypted hello from ");
            Serial.println(receivedPacket.addressFrom);
            Serial.print("Message: ");
            Serial.println(receivedPacket.message);
        }

        //send encrypted response
        Packet pairingPacket;
        createPacket(pairingPacket, HEADER_ENCRYPTED_HELLO, receivedPacket.addressFrom, SENSOR_ADDRESS, "Hello");
        uint8_t buffer[sizeof(Packet)];
        memcpy(buffer, &pairingPacket, sizeof(Packet));
        LoRa.beginPacket();
        for (int i = 0; i < sizeof(Packet); ++i) {
            LoRa.write(buffer[i]);
        }
        LoRa.endPacket();
        delay(2000); // Wait for response 2 seconds
    }
    // write to EEPROM

    // should be commented in the end
    uint8_t hashedKey[32]; // we have this hash from the previous step
    uint16_t serverAddress; // we have this address from the previous step

    EEPROM.write(SERVER_ADDRESS_ADDRESS, serverAddress >> 8);
    EEPROM.write(SERVER_ADDRESS_ADDRESS + 1, serverAddress & 0xFF);

    for (int i = 0; i < 32; i++) {
        EEPROM.write(SECRET_HASH_ADDRESS + i, hashedKey[i]);
    }

    EEPROM.write(IS_PAIRED_ADDRESS, Paired);

    Serial.println("Device paired successfully.");
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
        handlePairing();
    } else {
        pinMode(gpioPin, INPUT);
        int state = digitalRead(gpioPin);
        for (int i = 0; i < 100; ++i) {
            if (state == HIGH) {
                Serial.println("GPIO1 is HIGH. Read the address");
            } else {
                break;
            }
            delay(30);
            if (i == 99) {
                handlePairing();
            }
        }

        readEEPROM();
    }
}

void loop() {
//    if (isPaired != notPaired) {
    Packet packet;
    createPacket(packet, HEADER_MESSAGE, serverAddress, SENSOR_ADDRESS, "Hello World, this is Electronic Clinic");
    LoRa.beginPacket();
    LoRa.write((uint8_t*)&packet, sizeof(Packet));
    LoRa.endPacket();
    delay(1000); // Send message every second
//    }
}

}
