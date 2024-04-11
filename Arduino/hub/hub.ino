
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

#include <P521.h>
#include <Curve25519.h>
#include <SHA3.h>
#include <AES.h>
#include <base64.hpp>

#define SERVER_ADDRESS 0x00

#define HEADER_PAIRING_REQUEST 1
#define HEADER_PAIRING_RESPONSE 2
#define HEADER_PAIRING_PROCEDURE 3
#define HEADER_ENCRYPTED_HELLO 128
#define INFORMAL_MESSAGE 160
#define HEADER_MESSAGE 255

struct SensorStruct {
//    String name;
//    String type;
//    String units;
    uint8_t hashedKey[32];
};
std::map<uint16_t, SensorStruct> dataSPIFFS;

struct Packet {
    uint8_t magicByte;
    uint8_t header;
    uint16_t addressTo;
    uint16_t addressFrom;
    char message[58];
};

uint8_t privateKeyHub[32];
uint8_t publicKeyHub[32];
uint8_t combinedKey[32];
uint8_t hashedKey[32];


AES256 cipherBlock;
SHA3_256 sha3;

void createPacket(Packet& packet, uint8_t header, uint16_t addressTo, uint16_t addressFrom, const char* message) {
    packet.magicByte = 50;
    packet.header = header;
    packet.addressTo = addressTo;
    packet.addressFrom = addressFrom;
    strncpy(packet.message, message, 57);
    packet.message[57] = '\0';

}

void parsePacket(const uint8_t* buffer, Packet& packet) {
    memcpy(&packet, buffer, sizeof(Packet));
}



void generate_keys() {
    Serial.println("start generating keys for user1");
    Curve25519::dh1(publicKeyHub, privateKeyHub);
    Serial.println("generated the key");
    Serial.println("publicKey1: ");
    for(uint8_t elem: publicKeyHub){
        Serial.print(elem, HEX);
    }
    Serial.print("\n");

    Serial.println("privateKey1:");
    for(uint8_t elem: privateKeyHub){
        Serial.print(elem, HEX);
    }
    Serial.println("\n");
}


void mix_key(String publicKeyReceived) {
    // convert string to uint8_t
    for (int i = 0; i < 32; i++) {
        combinedKey[i] = publicKeyReceived.charAt(i);
    }
    Serial.println("derive shared key for user1");
    if(!Curve25519::dh2(combinedKey, privateKeyHub)){
        Serial.println("Couldn't derive shared key");
    };
    Serial.println("derived the key");

    Serial.println("sharedKey1: ");
    for (uint8_t elem : combinedKey) {
        Serial.print(elem, HEX);
    }
    Serial.println("\n");
}

void hash_key(){
    uint8_t encryptionKey[32];
    memcpy(encryptionKey, combinedKey, sizeof(combinedKey));
    sha3.update(encryptionKey, 32);
    sha3.finalize(hashedKey, 32);
    sha3.reset();
    for (uint8_t elem : hashedKey) {
        Serial.print(elem, HEX);
    }
}


uint8_t* encode_message(const char* text) {
    Serial.println("encode message");
    uint8_t gotText[58];
    for(int i = 0; text[i] != '\0' && i < 57; i++) {
        Serial.print(text[i]);
    }
    Serial.println();

    cipherBlock.setKey(hashedKey, 32);
    cipherBlock.encryptBlock(gotText, reinterpret_cast<const uint8_t*>(text));

    Serial.println("encrypted message:");
    for (int i = 0; i < 57; i++) {
        Serial.print((char)gotText[i]);
    }
    Serial.println("\n");
    return gotText;
}


char* decrypt_message(uint8_t* gotText) {
    Serial.println("decrypt message");
    uint8_t decrypted[58]; // Assuming size 58 for compatibility with your previous code

    cipherBlock.decryptBlock(decrypted, gotText);

    Serial.println("decrypted message should be : Hello, AES256!");
    for (int i = 0; i < 57; i++) {
        Serial.print((char)decrypted[i]); // Print each character of the decrypted message
    }
    Serial.println("\n");
    static char decrypted_char[58]; // Assuming size 58 for compatibility
    for (int i = 0; i < 57; i++) {
        decrypted_char[i] = (char)decrypted[i];
    }
    decrypted_char[57] = '\0'; // Null-terminate the string
    return decrypted_char;
}



void handlePairing(Packet& packet) {
    // first step
    // send public key to sensor
    if (packet.header == HEADER_PAIRING_REQUEST) {
        Serial.print("Received pairing request from ");
        Serial.println(packet.addressFrom);

        generate_keys();
        delay(1000);
        // generate public and private keys
        // maybe need convert key to string
        //convertion
        
        // Create a char array to hold the string
        char publicKeyHubString[33]; // +1 for null terminator
        
        // Copy the contents of publicKeyHub to publicKeyHubString
        for (int i = 0; i < 32; i++) {
            publicKeyHubString[i] = static_cast<char>(publicKeyHub[i]);
        }
        publicKeyHubString[32] = '\0'; // Null terminate the char array
        
        // send public key to sensor
        Packet responsePacket;
        Serial.println("created packet 1");
        createPacket(responsePacket, HEADER_PAIRING_RESPONSE, packet.addressFrom, SERVER_ADDRESS, publicKeyHubString);
        uint8_t buffer[sizeof(Packet)];
        memcpy(buffer, &responsePacket, sizeof(Packet));
        LoRa.beginPacket();
        for (int i = 0; i < sizeof(Packet); ++i) {
            LoRa.write(buffer[i]);
        }
        LoRa.endPacket();
        Serial.println("sent packet 1");
        delay(2000); // Wait for response 2 seconds
    }

    // second step
    // receive public key from sensor
    int packetSize = LoRa.parsePacket();
    Serial.println("trying to get packet 2");
    if (packetSize == sizeof(Packet)) {
        Packet receivedPacket;
        uint8_t buffer[sizeof(Packet)];
        for (int i = 0; i < sizeof(Packet); ++i) {
            buffer[i] = LoRa.read();
        }
        parsePacket(buffer, receivedPacket);
        Serial.println("before pairing procedure");
        if (receivedPacket.header == HEADER_PAIRING_PROCEDURE) {
            Serial.print("Received pairing response from ");
            Serial.println(receivedPacket.addressFrom);

            // save public key
            char* receivedPublicKey = receivedPacket.message;
            Serial.println("Received public key from server");
            Serial.println(receivedPublicKey);

            // combine keys and creating hash
            mix_key(receivedPublicKey);
            delay(5000);
            hash_key();
            delay(5000);
            
            // send encrypted hello
            Serial.println("created packet 2");
            uint8_t* cipherText = encode_message("Hello, AES256!");
            Packet pairingPacket;
            createPacket(pairingPacket, HEADER_ENCRYPTED_HELLO, receivedPacket.addressFrom, SERVER_ADDRESS, (char*)cipherText);
            uint8_t buffer[sizeof(Packet)];
            memcpy(buffer, &pairingPacket, sizeof(Packet));
            LoRa.beginPacket();
            for (int i = 0; i < sizeof(Packet); ++i) {
                LoRa.write(buffer[i]);
            }
            LoRa.endPacket();
            Serial.println("sent packet 2");
            delay(2000); // Wait for response 2 seconds

//            // write to SPIFFS
//            SensorStruct info;
//            memcpy(info.hashedKey, hashedKey, sizeof(hashedKey));
//        
//            dataSPIFFS[receivedPacket.addressFrom] = info;
//        
//            // Write to file in SPIFFS (assuming writeFile and appendFile functions are correctly implemented)
//            String path = "/" + String(receivedPacket.addressFrom);
//            writeFile(SPIFFS, path.c_str(), info);
//            appendFile(SPIFFS, path.c_str(), "\n");
//        
//        
//            // then add sensor option
//            // got from sensor with header 160
//              }
//            }

        }
    }

    

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
void parseInfoMessage(const char* message, uint16_t addressFrom) {
//    std::stringstream ss(message);
//    std::string token;
//    SensorStruct info;
//
//    // Read sensor name
//    std::getline(ss, token, ',');
//
//    info.hashedKey = token;

    SensorStruct info;
    memcpy(info.hashedKey, hashedKey, sizeof(hashedKey));


    // Add to sensor map
    dataSPIFFS[addressFrom] = info;

    // Write to file in SPIFFS (assuming writeFile and appendFile functions are correctly implemented)
    String path = "/" + String(addressFrom);
    writeFile(SPIFFS, path.c_str(), info);
    appendFile(SPIFFS, path.c_str(), "\n");
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
    listDir(SPIFFS, "/", 0);
    File root = SPIFFS.open("/");
    readFiles(SPIFFS, root);
    deleteFile(SPIFFS, "/0");
    root.close();
}

void loop() {
//    Serial.println("Waiting for message");
//    int packetSize = LoRa.parsePacket();
//    if (packetSize == sizeof(Packet)) {
//        Packet receivedPacket;
//        uint8_t buffer[sizeof(Packet)];
//        for (int i = 0; i < sizeof(Packet); ++i) {
//            buffer[i] = LoRa.read();
//        }
//        parsePacket(buffer, receivedPacket);
//        Serial.print("receivedPacket.addressFrom: ");
//        Serial.println(receivedPacket.addressFrom);
//        // check if my address or broadcast
//        if (receivedPacket.addressTo == SERVER_ADDRESS || receivedPacket.addressTo == 0xFFFF) {
//            if (receivedPacket.magicByte == 50) { // Check if it's our protocol
//                if (receivedPacket.header == HEADER_PAIRING_REQUEST) { // Pairing header
//                    handlePairing(receivedPacket);
//                    parseInfoMessage(receivedPacket.message, receivedPacket.addressFrom);
//                } else {
//                    // decrypt message
//                    // Assuming receivedPacket.message is a null-terminated char array
//
//                    // Calculate the length of the char array
//                    size_t messageLength = strlen(receivedPacket.message);
//                    
//                    // Allocate memory for the uint8_t array (plus one for the null terminator)
//                    uint8_t* uintMessage = new uint8_t[messageLength + 1];
//                    
//                    // Copy the characters from receivedPacket.message to uintMessage
//                    for (size_t i = 0; i <= messageLength; ++i) {
//                        uintMessage[i] = static_cast<uint8_t>(receivedPacket.message[i]);
//                    }
//                    
//                    // Pass uintMessage to decrypt_message
//                    char* decryptedMessage = decrypt_message(uintMessage);
//                    
//                    // Don't forget to free the allocated memory when you're done
//                    delete[] uintMessage;
//
//                    Serial.print("Received message: ");
//                    Serial.println(decryptedMessage);
//                    Serial.print("From: ");
//                    Serial.println(receivedPacket.addressFrom);
//                }
//            }
//        }
//    }
//    delay(100);
}
