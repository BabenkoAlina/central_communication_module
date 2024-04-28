
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

std::map<uint16_t, SensorStruct> dataSPIFFS;

uint8_t privateKeyHub[32];
uint8_t publicKeyHub[32];
uint8_t combinedKey[32];
uint8_t hashedKey[32];


AES256 cipherBlock;
SHA3_256 sha3;

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
    Serial.println("start generating keys for user1");
    Curve25519::dh1(publicKeyHub, privateKeyHub);
    Serial.println("generated the key");

    Serial.println("publicKey: ");
    encode_base64(publicKeyHub, 32, base64_buffer);
    Serial.println((char *) base64_buffer);
    Serial.print("\n");

    Serial.println("privateKey:");
    encode_base64(privateKeyHub, 32, base64_buffer);
    Serial.println((char *) base64_buffer);
    Serial.println("\n");
}


void mix_key(uint8_t* publicKeyReceived) {
    Serial.println("derive shared key for user1");
    if(!Curve25519::dh2(combinedKey, privateKeyHub)){
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






void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\r\n", dirname);
    File root = fs.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return;
    }
    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void writeFile(fs::FS &fs, const char * path, SensorStruct sensor){
    Serial.printf("Writing file: %s\r\n", path);
    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return;
    }
    String hashedKeyString(reinterpret_cast<char*>(sensor.hashedKey));
//    String hashedKeyString = (char*) sensor.hashedKey;

    if (file.print(hashedKeyString)) {
        Serial.println("- file written");
    } else {
        Serial.println("- write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\r\n", path);
    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("- failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("- message appended");
    } else {
        Serial.println("- append failed");
    }
    file.close();
}

void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\r\n", path);
    if(fs.remove(path)){
        Serial.println("- file deleted");
    } else {
        Serial.println("- delete failed");
    }
}

void readFiles(fs::FS &fs, File root) {
    File file = root.openNextFile();
    while (file) {
        String path = file.name();
        Serial.printf("Reading file: %s\r\n", path.c_str());

        if (file.isDirectory()) {
            Serial.println("- Skipping directory");
        } else {
            // Read address from file name
            String fileName = file.name();
            int lastIndex = fileName.lastIndexOf('/');
            String addressString = fileName.substring(lastIndex + 1);
            uint16_t address = addressString.toInt();

            SensorStruct sensor;

            String hashedKeyString = file.readStringUntil('\n');
            if (hashedKeyString.length() >= 32) {
                for (int i = 0; i < 32; i++) {
                    sensor.hashedKey[i] = (uint8_t)hashedKeyString.charAt(i);
                }
            }

            Serial.print("Address: ");
            Serial.println(address);
            Serial.print("Hashed key: ");
            for (int i = 0; i < 32; i++) {
                Serial.print(sensor.hashedKey[i], HEX); // Print each byte of the hashed key in hexadecimal format
            }
            Serial.println();

            dataSPIFFS[address] = sensor;
        }

        file = root.openNextFile();
    }
}

void writeSPIFFS(const uint8_t* message, uint16_t sensorAddress) {
    uint8_t *decodedMessage = decrypt_message(message);
    SensorStruct info;
    memcpy(info.hashedKey, hashedKey, sizeof(hashedKey));

    // split uint8_t* message by coma into type and measurement
    char type[50];
    char measurement[50];
    sscanf((const char*)decodedMessage, "%[^,], %[^\n]", type, measurement);
    strncpy(info.type, type, sizeof(info.type));
    strncpy(info.measurement, measurement, sizeof(info.measurement));

    dataSPIFFS[sensorAddress] = info;

    // Write to file in SPIFFS (assuming writeFile and appendFile functions are correctly implemented)
    String path = "/" + String(sensorAddress);
    writeFile(SPIFFS, path.c_str(), info);
    appendFile(SPIFFS, path.c_str(), "\n");
}




void handlePairing(Packet& packet) {
    uint16_t sensorAddress = packet.addressFrom;

    uint8_t receivedPublicKey[32];

    // sending key //////////////////////////////////////////////////////
    Serial.println("Sending key to sensor");
    generate_keys();
    while (true) {
        Packet responsePacket;
        createPacket(responsePacket, HEADER_PAIRING_RESPONSE, sensorAddress, SERVER_ADDRESS, publicKeyHub);
        uint8_t buffer[sizeof(Packet)];
        memcpy(buffer, &responsePacket, sizeof(Packet));
        LoRa.beginPacket();
        for (int i = 0; i < sizeof(Packet); ++i) {
            LoRa.write(buffer[i]);
        }
        LoRa.endPacket();

        // receive public key from sensor //////////////////////////////////
        int packetSize = LoRa.parsePacket();
        if (packetSize == sizeof(Packet)) {
            Packet receivedPacket;
            uint8_t buffer[sizeof(Packet)];
            for (int i = 0; i < sizeof(Packet); ++i) {
                buffer[i] = LoRa.read();
            }
            parsePacket(buffer, receivedPacket);
            Serial.println("before pairing procedure");
            if (receivedPacket.addressTo == SERVER_ADDRESS && receivedPacket.header == HEADER_PAIRING_PROCEDURE &&
                receivedPacket.magicByte == 50) {
                memcpy(receivedPublicKey, receivedPacket.message, 32);

                Serial.println("Received public key");
                for (uint8_t elem : receivedPublicKey) {
                    Serial.print(elem, HEX);
                }
                break;
            }
        }
    }

    mix_key(receivedPublicKey);

    hash_key();

    // send encrypted hello /////////////////////////////////////////////
    while (true) {
        uint8_t *cipherText = encode_message(reinterpret_cast<const uint8_t*>("Hello, Sensor!"));

        Serial.println("cipherText:");
        encode_base64(cipherText, 32, base64_buffer);
        Serial.println((char *) base64_buffer);
        Serial.print("\n");

        Packet packet;
        createPacket(packet, HEADER_ENCRYPTED_HELLO, sensorAddress, SERVER_ADDRESS, cipherText);
        uint8_t buffer[sizeof(Packet)];
        memcpy(buffer, &packet, sizeof(Packet));
        LoRa.beginPacket();
        for (int i = 0; i < sizeof(Packet); ++i) {
            LoRa.write(buffer[i]);
        }
        LoRa.endPacket();

        // receive sensor info //////////////////////////////////////////////
        int packetSize = LoRa.parsePacket();
        if (packetSize == sizeof(Packet)) {
            Packet receivedPacket;
            uint8_t buffer[sizeof(Packet)];
            for (int i = 0; i < sizeof(Packet); ++i) {
                buffer[i] = LoRa.read();
            }
            parsePacket(buffer, receivedPacket);
            if (receivedPacket.addressTo == SERVER_ADDRESS && receivedPacket.header == HEADER_INFORMAL_MESSAGE && receivedPacket.magicByte == 50) {

                Serial.println("sensor info:");
                for (int i = 0; i < 57; i++) {
                    Serial.print((char) receivedPacket.message[i]);
                }

//                writeSPIFFS(receivedPacket.message, sensorAddress);
                break;
            }
        }
    }
}






void setup() {
    Serial.begin(9600);
//  (int ss, int reset, int dio0)
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
//    listDir(SPIFFS, "/", 0);
//    File root = SPIFFS.open("/");
//    readFiles(SPIFFS, root);
//    deleteFile(SPIFFS, "/1");
//    listDir(SPIFFS, "/", 0);
//    root.close();
}

void loop() {
    Serial.println("Waiting for message");
    int packetSize = LoRa.parsePacket();
    if (packetSize == sizeof(Packet)) {
        Packet receivedPacket;
        uint8_t buffer[sizeof(Packet)];
        for (int i = 0; i < sizeof(Packet); ++i) {
            buffer[i] = LoRa.read();
        }
        parsePacket(buffer, receivedPacket);

        if (receivedPacket.addressTo == SERVER_ADDRESS || receivedPacket.addressTo == 0xFFFF) {
            if (receivedPacket.magicByte == 50) { // Check if it's our protocol
                if (receivedPacket.header == HEADER_PAIRING_REQUEST) { // Pairing header
                    handlePairing(receivedPacket);
                } else {
                    uint8_t* decryptedMessage = decrypt_message(receivedPacket.message);

                    for (int i = 0; i < sizeof(decryptedMessage); ++i) {
                        Serial.print((char)decryptedMessage[i]);
                    }
                }
            }
        }
    }
    delay(100);
}
