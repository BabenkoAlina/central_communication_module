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
    memcpy(packet.message, message, 58);
}


void parsePacket(const uint8_t* buffer, Packet& packet) {
    memcpy(&packet, buffer, sizeof(Packet));
}


void generate_keys() {
    Serial.println("start generating keys");
    Curve25519::dh1(publicKeySensor, privateKeySensor);
    Serial.println("generated the key");

    Serial.println("publicKey: ");
    encode_base64(publicKeySensor, 32, base64_buffer);
    Serial.println((char *) base64_buffer);
    Serial.print("\n");

    Serial.println("privateKey:");
    encode_base64(privateKeySensor, 32, base64_buffer);
    Serial.println((char *) base64_buffer);
    Serial.print("\n");
}


void mix_key(uint8_t* publicKeyReceived) {
    Serial.println("derive shared key for user1");
    if(!Curve25519::dh2(combinedKey, privateKeySensor)){
        Serial.println("Couldn't derive shared key");
    };
    Serial.println("derived the key");

    Serial.println("mixed key: ");
    encode_base64(combinedKey, 32, base64_buffer);
    Serial.println((char *) base64_buffer);
    Serial.print("\n");
}



void hash_key(){
    uint8_t encryptionKey[32];
    memcpy(encryptionKey, combinedKey, sizeof(combinedKey));
    sha3.update(encryptionKey, 32);
    sha3.finalize(hashedKey, 32);
    sha3.reset();
//    for (uint8_t elem : hashedKey) {
//        Serial.print(elem, HEX);
//    }
    Serial.println("hashed key: ");
    encode_base64(encryptionKey, 32, base64_buffer);
    Serial.println((char *) base64_buffer);
    Serial.print("\n");
}


uint8_t* encode_message(const uint8_t* text) {
    Serial.println("encode message");

    uint8_t encodedText[58];
    cipherBlock.setKey(hashedKey, 32);
    cipherBlock.encryptBlock(encodedText, text); // , 57);

    encode_base64(encodedText, 32, base64_buffer);
    Serial.println((char *) base64_buffer);
    Serial.print("\n");

    return encodedText;
}


uint8_t* decrypt_message(const uint8_t* gotText) {
    Serial.println("decrypt message");
    uint8_t decrypted[58];

    cipherBlock.decryptBlock(decrypted, gotText);
    encode_base64(decrypted, 32, base64_buffer);
    Serial.println((char *) base64_buffer);
    Serial.print("\n");
    return decrypted;
}




void readEEPROM(){
    Serial.println(EEPROM.read(0));
    serverAddress = EEPROM.read(SERVER_ADDRESS) << 8 | EEPROM.read(SERVER_ADDRESS + 1);
    Serial.print("Server Address: ");
    Serial.println(serverAddress);

    Serial.println("Secret Hash:");
    for (int i = 0; i < 32; i++) {
        secretHashBytes[i] = EEPROM.read(SECRET_HASH_ADDRESS + i);
        Serial.print(secretHashBytes[i], HEX);
    }
    Serial.println();
}

void writeEEPROM() {
    EEPROM.write(IS_PAIRED_ADDRESS, Paired);

    EEPROM.write(SERVER_ADDRESS, serverAddress >> 8);
    EEPROM.write(SERVER_ADDRESS + 1, serverAddress & 0xFF);

    for (int i = 0; i < 32; i++) {
        EEPROM.write(SECRET_HASH_ADDRESS + i, hashedKey[i]);
    }
}


void handlePairing() {
    uint8_t receivedPublicKey[32];
    while (true) {
        // broadcast request /////////////////////////////////////////////
        Serial.println("Sending broadcast request");
        Packet packet;
        createPacket(packet, HEADER_PAIRING_REQUEST, 0xFFFF, SENSOR_ADDRESS, reinterpret_cast<const uint8_t*>("Requesting pairing"));
        uint8_t buffer[sizeof(Packet)];
        memcpy(buffer, &packet, sizeof(Packet));
        LoRa.beginPacket();
        for (int i = 0; i < sizeof(Packet); ++i) {
            LoRa.write(buffer[i]);
        }
        LoRa.endPacket();

        // get key ///////////////////////////////////////////////////////
        Serial.println("Getting response");
        int packetSize = LoRa.parsePacket();
        if (packetSize == sizeof(Packet)) {
            Packet receivedPacket;
            uint8_t buffer[sizeof(Packet)];
            for (int i = 0; i < sizeof(Packet); ++i) {
                buffer[i] = LoRa.read();
            }
            parsePacket(buffer, receivedPacket);
            if (receivedPacket.addressTo == SENSOR_ADDRESS && receivedPacket.header == HEADER_PAIRING_RESPONSE &&
                receivedPacket.magicByte == 50) {
                memcpy(receivedPublicKey, receivedPacket.message, 32);
                serverAddress = receivedPacket.addressFrom;

                Serial.println("Received public key");
                for (uint8_t elem : receivedPublicKey) {
                    Serial.print(elem, HEX);
                }
                break;
            }
        }
    }

    generate_keys();

    mix_key(receivedPublicKey);

    hash_key();

    // sending public key ////////////////////////////////////////////
    while (true) {
        Packet packet;
        createPacket(packet, HEADER_PAIRING_PROCEDURE, serverAddress, SENSOR_ADDRESS, publicKeySensor);
        uint8_t buffer[sizeof(Packet)];
        memcpy(buffer, &packet, sizeof(Packet));
        LoRa.beginPacket();
        for (int i = 0; i < sizeof(Packet); ++i) {
            LoRa.write(buffer[i]);
        }
        LoRa.endPacket();

        // get encrypted hello //////////////////////////////////////////
        int packetSize = LoRa.parsePacket();
        if (packetSize == sizeof(Packet)) {
            Packet receivedPacket;
            uint8_t buffer[sizeof(Packet)];
            for (int i = 0; i < sizeof(Packet); ++i) {
                buffer[i] = LoRa.read();
            }
            parsePacket(buffer, receivedPacket);
            Serial.println("before pairing procedure");
            if (receivedPacket.addressTo == SERVER_ADDRESS && receivedPacket.header == HEADER_ENCRYPTED_HELLO &&
                receivedPacket.magicByte == 50) {
                uint8_t *decrypted = decrypt_message(receivedPacket.message);
                for (int i = 0; i < 57; i++) {
                    Serial.print((char) decrypted[i]);
                }
                break;
            }
        }
    }


    //send inform message////////////////////////////////////////////
    for (int i = 0; i < 10; i++) {
        Packet packet;
        uint8_t infoMessage[] = "Temperature, Celsius";
        uint8_t *cipherText = encode_message(infoMessage);

        Serial.println("info text:");
        encode_base64(cipherText, 32, base64_buffer);
        Serial.println((char *) base64_buffer);
        Serial.print("\n");

        createPacket(packet, HEADER_INFORMAL_MESSAGE, serverAddress, SENSOR_ADDRESS, cipherText);
        uint8_t buffer[sizeof(Packet)];
        memcpy(buffer, &packet, sizeof(Packet));
        LoRa.beginPacket();
        for (int i = 0; i < sizeof(Packet); ++i) {
            LoRa.write(buffer[i]);
        }
        LoRa.endPacket();
    }

//    writeEEPROM();
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
//    readEEPROM();
//    EEPROM.write(0,32);
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
//        readEEPROM();
    }
}

void loop() {
    Packet packet;
    uint8_t message[] = "Hello, world!";
    uint8_t* cipherText = encode_message(message);
    createPacket(packet, HEADER_MESSAGE, serverAddress, SENSOR_ADDRESS, cipherText);
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
