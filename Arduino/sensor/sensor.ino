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

#include "DHT.h"
#define DHT_PIN 2
#define DHT_TYPE DHT11
DHT dht11(DHT_PIN, DHT_TYPE);

#define SENSOR_ADDRESS 0x02

#define IS_PAIRED_ADDRESS 0
#define SERVER_ADDRESS 0x05
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
uint8_t decryptedText[58];
uint8_t encodedText[58];

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

    // Ensure message is null-terminated
    uint8_t messageBuffer[sizeof(packet.message)];
    memset(messageBuffer, 0, sizeof(messageBuffer));
    memcpy(messageBuffer, message, sizeof(messageBuffer) - 1); // Copy message, leaving space for null-terminator

    strncpy((char*)packet.message, (const char*)messageBuffer, sizeof(packet.message));
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
    if(!Curve25519::dh2(publicKeyReceived, privateKeySensor)){
        Serial.println("Couldn't derive shared key");
    };
    Serial.println("derived the key");
    memcpy(hashedKey, publicKeyReceived, 32);

    // Serial.println("mixed key: ");
    // encode_base64(combinedKey, 32, base64_buffer);
    // Serial.println((char *) base64_buffer);
    // Serial.print("\n");
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
    // Serial.println("hashed key: ");
    // encode_base64(encryptionKey, 32, base64_buffer);
    // Serial.println((char *) base64_buffer);
    // Serial.print("\n");
}


// uint8_t* encode_message(const uint8_t* text) {
//     Serial.println("encode message");

//     uint8_t encodedText[58];
//     cipherBlock.setKey(hashedKey, 32);
//     cipherBlock.encryptBlock(encodedText, text); // , 57);

//     Serial.println((char *) encodedText);
//     encode_base64(encodedText, 32, base64_buffer);
//     Serial.println((char *) base64_buffer);
//   for (int i=0; i<58; i++){
//      Serial.print((char)encodedText[i]);
//   }
//     Serial.print("\n");

//     return base64_buffer;
// }


void encode_message(const uint8_t* text) {
    // for(int i =0; i<32; ++i){
    //   Serial.print(text[i]);
    // }
    // Serial.println("encode message");

    cipherBlock.setKey(hashedKey, 32);
    cipherBlock.encryptBlock(encodedText, text); // , 57);
    // for(int i =0; i<32; ++i){
    //   Serial.print(encodedText[i]);
    // }

    //encode_base64(encodedText, 32, base64_buffer);

    //Serial.println((char *) base64_buffer);
    // for(int i =0; i<32; ++i){
    //   Serial.print(encodedText[i]);
    // }
    // Serial.print("\n");
}

void decrypt_message() {
    // for(int i =0; i<32; ++i){
    //   Serial.print(encodedText[i]);
    // }
    // Serial.println("decrypt message");

    cipherBlock.setKey(hashedKey, 32);
    cipherBlock.decryptBlock(decryptedText, encodedText);
    //encode_base64(decrypted, 32, base64_buffer);
    // for(int i =0; i<32; ++i){
    //   Serial.print(decryptedText[i]);
    // }
    // Serial.print("\n");
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
        uint8_t message[]= "Requesting pairing";
        Serial.println("Sending broadcast request");
        Packet packet;
        createPacket(packet, HEADER_PAIRING_REQUEST, 0xFFFF, SENSOR_ADDRESS, message);
        uint8_t buffer[sizeof(Packet)];
        memcpy(buffer, &packet, sizeof(Packet));
        LoRa.beginPacket();
        LoRa.write(buffer, sizeof(Packet));
        LoRa.endPacket();

        // get key ///////////////////////////////////////////////////////
        Serial.println("Getting response");
        delay(2000);
        int packetSize = LoRa.parsePacket();
        //Serial.println(packetSize);
        if (packetSize >0 && packetSize<=sizeof(Packet)) {
            Serial.println("got some");
            Packet receivedPacket;
            uint8_t buffer[sizeof(Packet)];
            // for (int i = 0; i < sizeof(Packet); ++i) {
            //     buffer[i] = LoRa.read();
            // }
            LoRa.readBytes(buffer, packetSize);

            parsePacket(buffer, receivedPacket);
            if (receivedPacket.addressTo == SENSOR_ADDRESS && receivedPacket.header == HEADER_PAIRING_RESPONSE &&
                receivedPacket.magicByte == 50) {
                memcpy(receivedPublicKey, receivedPacket.message, 32);
                serverAddress = receivedPacket.addressFrom;

                // Serial.println("Received public key");
                // for (uint8_t elem : receivedPublicKey) {
                //     Serial.print(elem, HEX);
                // }
                break;
            }
        }
    }

    generate_keys();

    mix_key(receivedPublicKey);

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

        delay(1000);
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
                memcpy(encodedText, receivedPacket.message, 58);
                decrypt_message();
                String text = String((char*)decryptedText);
                Serial.println(text);
                break;
            }
        }
    }


    //send inform message////////////////////////////////////////////
    for (int i = 0; i < 10; i++) {
        Packet packet;
        uint8_t infoMessage[] = "Temperature, Celsius";
        encode_message(infoMessage);

        // Serial.println("info text:");
        // encode_base64(cipherText, 32, base64_buffer);
        // Serial.println((char *) base64_buffer);
        // Serial.print("\n");

        createPacket(packet, HEADER_INFORMAL_MESSAGE, serverAddress, SENSOR_ADDRESS, encodedText);
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
    LoRa.enableCrc();
    dht11.begin();
    

   readEEPROM();
   EEPROM.write(0,32);
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
  //generate_keys();
    float temperature_C = dht11.readTemperature();
    float humi  = dht11.readHumidity();

    if (isnan(temperature_C) || isnan(humi)) {
        Serial.println("Failed to read from DHT sensor!");
    } else {
        Serial.print("Humidity: ");
        Serial.print(humi);
        Serial.print("%");

        Serial.print("  |  ");

        Serial.print("Temperature: ");
        Serial.println(temperature_C);
    }
    
    uint8_t temperature[4];
    // convert float to uint8_t array
    if (SENSOR_ADDRESS == 0x01){
      memcpy(temperature, &temperature_C, 4);
    }else if (SENSOR_ADDRESS == 0x02){
      memcpy(temperature, &humi, 4);
    }


  //   Serial.println("Temperature bytes:");
  //   for (int i = 0; i < sizeof(temperature); i++) {
  //     Serial.print(temperature[i]);
  //     Serial.print(" ");
  //     // Serial.print(temperature[i]);
  //     // Serial.print(" ");
  //   }
  // Serial.println();
  encode_message(temperature);
  // decrypt_message();
  // float decodedTemperature;
  // memcpy(&decodedTemperature, temperature, sizeof(float));
  // Serial.print("Decoded temperature original: ");
  // Serial.println(decodedTemperature);
  // memcpy(&decodedTemperature, decryptedText, sizeof(float));
  // Serial.print("Decoded temperature: ");
  // Serial.println(decodedTemperature);
  // for (int i=0; i<58; i++){
  //    Serial.print((char)decryptedText[i]);
  // }
 
    Packet packet;
    createPacket(packet, HEADER_MESSAGE, SERVER_ADDRESS, SENSOR_ADDRESS, encodedText);
    uint8_t buffer[sizeof(Packet)];
    memcpy(buffer, &packet, sizeof(Packet));

    int a = LoRa.beginPacket();
    // Serial.println(a);

    LoRa.write(buffer, sizeof(Packet));
    LoRa.endPacket();
    delay(1500);
}
