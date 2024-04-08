
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

#define SERVER_ADDRESS 0x00

//[1 byte] - header (1 - request for pairing, 2 - response for pairing, 3-pairing procedure, 128-encrypted hello, 255-message)
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
    std::string hashedKey;
};
std::map<uint16_t, std::string> dataSPIFFS; // Use address as key

struct Packet {
    uint8_t magicByte;
    uint8_t header;
    uint16_t addressTo;
    uint16_t addressFrom;
    char message[58];
};

uint8_t privateKeyHub[66];
uint8_t publicKeyHub[132];

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



void handlePairing(Packet& packet) {
    // first step
    // send public key to sensor
    if (packet.header == HEADER_PAIRING_REQUEST) {
        Serial.print("Received pairing request from ");
        Serial.println(packet.addressFrom);

        // generate public and private keys
        // maybe need convert key to string
        ...

        // send public key to sensor
        Packet responsePacket;
        createPacket(responsePacket, HEADER_PAIRING_RESPONSE, packet.addressFrom, SERVER_ADDRESS, publicKeyHub);
        uint8_t buffer[sizeof(Packet)];
        memcpy(buffer, &responsePacket, sizeof(Packet));
        LoRa.beginPacket();
        for (int i = 0; i < sizeof(Packet); ++i) {
            LoRa.write(buffer[i]);
        }
        LoRa.endPacket();
        delay(2000); // Wait for response 2 seconds
    }

    // second step
    // receive public key from sensor
    int packetSize = LoRa.parsePacket();
    if (packetSize == sizeof(Packet)) {
        Packet receivedPacket;
        uint8_t buffer[sizeof(Packet)];
        for (int i = 0; i < sizeof(Packet); ++i) {
            buffer[i] = LoRa.read();
        }
        parsePacket(buffer, receivedPacket);
        if (receivedPacket.header == HEADER_PAIRING_PROCEDURE) {
            Serial.print("Received pairing response from ");
            Serial.println(receivedPacket.addressFrom);

            // save public key
            ...
            // combine keys and creating hash
            ...

            // send encrypted hello
            Packet pairingPacket;
            createPacket(pairingPacket, HEADER_ENCRYPTED_HELLO, receivedPacket.addressFrom, SERVER_ADDRESS, "Hello");
            uint8_t buffer[sizeof(Packet)];
            memcpy(buffer, &pairingPacket, sizeof(Packet));
            LoRa.beginPacket();
            for (int i = 0; i < sizeof(Packet); ++i) {
                LoRa.write(buffer[i]);
            }
            LoRa.endPacket();
            delay(2000); // Wait for response 2 seconds
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

        // write to SPIFFS
        SensorStruct info;
        info.hashedKey = hashedKey; // got and store earlier // Natalia part
        dataSPIFFS[] = info;

        // Write to file in SPIFFS (assuming writeFile and appendFile functions are correctly implemented)
        String path = "/" + String(receivedPacket.addressFrom);
        writeFile(SPIFFS, path.c_str(), info);
        appendFile(SPIFFS, path.c_str(), "\n");
    }

    // then add sensor option
    // got from sensor with header 160

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
    if(file.print(sensor.hashedKey.c_str())){
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
            sensor.hashedKey = file.readStringUntil('\n').c_str();
            Serial.print("Address: ");
            Serial.println(address);
            Serial.print("Hashed key: ");
            Serial.println(sensor.hashedKey.c_str());

            dataSPIFFS[address] = sensor;
        }

        file = root.openNextFile();
    }
}
void parseInfoMessage(const char* message, uint16_t addressFrom) {
    std::stringstream ss(message);
    std::string token;
    SensorStruct info;

    // Read sensor name
    std::getline(ss, token, ',');

    info.hashedKey = token;

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
    root.close();
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
        // check if my address or broadcast
        if (receivedPacket.addressTo == SERVER_ADDRESS || receivedPacket.addressTo == 0xFFFF) {
            if (receivedPacket.magicByte == 50) { // Check if it's our protocol
                if (receivedPacket.header == HEADER_PAIRING_REQUEST) { // Pairing header
                    handlePairing(receivedPacket);
                } else {
                    Serial.print("Received message: ");
                    Serial.println(receivedPacket.message);
                    Serial.print("From: ");
                    Serial.println(receivedPacket.addressFrom);
                }
            }
        }

    }
}
