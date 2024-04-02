
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
#include <map>
#include <vector>
#include <string>

#include <SPI.h>
#include <LoRa.h>
#include "FS.h"
#include "SPIFFS.h"

String inString = "";
String MyMessage = "";



struct SensorStruct {
//    String name;
//    String type;
//    String units;
    String hashedKey;
};

std::map<uint16_t, SensorStruct> dataSPIFFS;

#define INFORMAL_MESSAGE 0b10100000


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

void readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\r\n", path);
    File file = fs.open(path);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        return;
    }
    Serial.println("- read from file:");
    while(file.available()){
        Serial.write(file.read());
    }
    file.close();
}


void writeFile(fs::FS &fs, const char * path, SensorStruct sensor){
    Serial.printf("Writing file: %s\r\n", path);
    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return;
    }
    if(file.print(sensor.hashedKey.c_str()){
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
void renameFile(fs::FS &fs, const char * path1, const char * path2){
    Serial.printf("Renaming file %s to %s\r\n", path1, path2);
    if (fs.rename(path1, path2)) {
        Serial.println("- file renamed");
    } else {
        Serial.println("- rename failed");
    }
}
void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\r\n", path);
    if(fs.remove(path)){
        Serial.println("- file deleted");
    } else {
        Serial.println("- delete failed");
    }
}
void testFileIO(fs::FS &fs, const char * path){
    Serial.printf("Testing file I/O with %s\r\n", path);
    static uint8_t buf[512];
    size_t len = 0;
    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return;
    }
    size_t i;
    Serial.print("- writing" );
    uint32_t start = millis();
    for(i=0; i<2048; i++){
        if ((i & 0x001F) == 0x001F){
          Serial.print(".");
        }
        file.write(buf, 512);
    }
    Serial.println("");
    uint32_t end = millis() - start;
    Serial.printf(" - %u bytes written in %u ms\r\n", 2048 * 512, end);
    file.close();
    file = fs.open(path);
    start = millis();
    end = start;
    i = 0;
    if(file && !file.isDirectory()){
        len = file.size();
        size_t flen = len;
        start = millis();
        Serial.print("- reading" );
        while(len){
            size_t toRead = len;
            if(toRead > 512){
                toRead = 512;
            }
            file.read(buf, toRead);
            if ((i++ & 0x001F) == 0x001F){
              Serial.print(".");
            }
            len -= toRead;
        }
        Serial.println("");
        end = millis() - start;
        Serial.printf("- %u bytes read in %u ms\r\n", flen, end);
        file.close();
    } else {
        Serial.println("- failed to open file for reading");
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
    SensorInfo info;

    // Read sensor name
    std::getline(ss, token, ',');
    info.sensorName = token;

    // Add to sensor map
    sensorMap[addressFrom] = info;

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

  // Read data from SPIFFS and store it in a vector
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
  if (packetSize) {
    while (LoRa.available())
    {
      int inChar = LoRa.read();
      inString += (char)inChar;
      MyMessage = inString;
    }
    inString = "";
    LoRa.packetRssi();


    // get from protocol
      uint8_t receivedMessage[64];

      uint8_t magicByte = receivedMessage[0];
      uint8_t header = receivedMessage[1];
      uint16_t addressTo = (receivedMessage[2] << 8) | receivedMessage[3];
      uint16_t addressFrom = (receivedMessage[4] << 8) | receivedMessage[5];
      char message[59];
      for (int i = 0; i < 58; i++) {
          message[i] = receivedMessage[6 + i];
      }
      message[58] = '\0';

      // тут має бути hash key, але треба продумати де саме він передастся
      if (header == INFORMAL_MESSAGE) {
          parseInfoMessage(message, addressFrom);
      }

//    Serial.println(MyMessage);
  }
}
