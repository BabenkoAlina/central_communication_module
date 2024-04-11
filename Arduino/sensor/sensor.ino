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

#include <P521.h>
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
#define INFORMAL_MESSAGE 160
#define HEADER_MESSAGE 255


uint16_t serverAddress;
uint8_t secretHashBytes[32];
byte isPaired;

uint8_t privateKeySensor[32];
uint8_t publicKeySensor[32];

uint8_t combinedKey[32];
uint8_t hashedKey[32];

uint8_t decryptedData[32];
//uint8_t plainText[] = "Hello, AES256!";
//uint8_t cipherText[sizeof(plainText)];
//uint8_t decrypted[sizeof(plainText)];
AES256 cipherBlock;
SHA3_256 sha3;

int gpioPin = 0;

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


void generate_keys() {
    Serial.println("start generating keys for user1");
    Curve25519::dh1(publicKeySensor, privateKeySensor);
    Serial.println("generated the key");
    Serial.println("publicKey1: ");
    //base64_buffer[0] = 0;
    //base64_length = encode_base64(publicKey1, 132, base64_buffer);
    //Serial.println(base64_length);
    //Serial.println((char *) base64_buffer);
    for(uint8_t elem: publicKeySensor){
        Serial.print(elem, HEX);
    }
    Serial.print("\n");

    Serial.println("privateKey1:");
    for(uint8_t elem: privateKeySensor){
        Serial.print(elem, HEX);
    }
    Serial.println("\n");
    //base64_length = encode_base64(privateKey1, 66, base64);
    //Serial.println((char *) base64);

    //sign_key();
}


void mix_key(String publicKeyReceived) {
    //sign()
    // convert string to uint8_t
//    for (int i = 0; i < 32; i++) {
//        combinedKey[i] = publicKeyReceived.charAt(i);
//    }
    Serial.println("derive shared key for user1");
    if(!Curve25519::dh2(combinedKey, privateKeySensor)){
        Serial.println("Couldn't derive shared key");
    };
    Serial.println("derived the key");

    Serial.println("sharedKey1: ");
    for (uint8_t elem : combinedKey) {
        Serial.print(elem, HEX);
    }
    Serial.println("\n");
    //base64_length = encode_base64(privateKey1, 66, base64_buffer);
    //Serial.println((char *) base64);
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
    uint8_t* gotText = (uint8_t*)malloc(58 * sizeof(uint8_t)); // Allocate memory dynamically
    if (!gotText) {
        Serial.println("Memory allocation failed!");
        return nullptr;
    }

    for(int i = 0; text[i] != '\0' && i < 57; i++) {
        Serial.print(text[i]); // Print each character of the input text
    }
    Serial.println();

    cipherBlock.setKey(hashedKey, 32);
    cipherBlock.encryptBlock(gotText, reinterpret_cast<const uint8_t*>(text));

    Serial.println("encrypted message:");
    for (int i = 0; i < 57; i++) {
        Serial.print((char)gotText[i]); // Print each character of the encrypted message
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


void handlePairing() {
    // first step
    // send pairing request

    // придумати де взяти адресу сервера!!!!!!!!!
    // broadcast
//    while (true) {
        Serial.println("Sending broadcast request");
        Packet packet;
        createPacket(packet, HEADER_PAIRING_REQUEST, 0xFFFF, SENSOR_ADDRESS, "Requesting pairing");
        LoRa.beginPacket();
        LoRa.write((uint8_t*)&packet, sizeof(Packet));
        LoRa.endPacket();
        Serial.println("Send broadcast request");
//        delay(10000); // Wait for response 3 seconds

        // second step
        // pair procedure

        // get public key from server
        Serial.println("getting responce from hub");
        
      
        while (true) {
        int packetSize = LoRa.parsePacket();
        Serial.println(packetSize);
        if (packetSize == sizeof(Packet)) {
            Packet receivedPacket;
            uint8_t buffer[sizeof(Packet)];
            for (int i = 0; i < sizeof(Packet); ++i) {
                buffer[i] = LoRa.read();
            }
            parsePacket(buffer, receivedPacket);
            Serial.println("Receiving public key");
            Serial.println(receivedPacket.addressTo);
            Serial.println(receivedPacket.addressFrom);
            Serial.println(receivedPacket.magicByte);
            Serial.println(receivedPacket.header);

            // check if my address or broadcast
            if (receivedPacket.addressTo == SENSOR_ADDRESS) {
                if (receivedPacket.magicByte == 50) { // Check if it's our protocol
                    if (receivedPacket.header == HEADER_PAIRING_RESPONSE) { // Pairing header
                        char* receivedPublicKey = receivedPacket.message;
                        Serial.println("Received public key from server");
                        uint8_t receivedPublicKey1[32];
                        for (int i = 0; i < 32; i++) {
                          receivedPublicKey1[i] = (uint8_t)receivedPublicKey[i];
                        }
                        Serial.println(receivedPublicKey);
                        serverAddress = receivedPacket.addressFrom;
                        
                        generate_keys();
                        
                        break;
                        delay(5000);
                        Serial.println("generated keys");
                        // generate public and private keys
                        // maybe need convert key to string
                        //convertion
                        
                        char publicKeySensorString[33]; // +1 for null terminator

                        // Copy the contents of publicKeyHub to publicKeyHubString
                        for (int i = 0; i < 32; i++) {
                            publicKeySensorString[i] = static_cast<char>(publicKeySensor[i]);
                        }
                        publicKeySensorString[32] = '\0'; // Null terminate the char array

                        // send public key to server
                        Packet responsePacket;
                        createPacket(responsePacket, HEADER_PAIRING_PROCEDURE, serverAddress, SENSOR_ADDRESS,
                                     publicKeySensorString);
                        uint8_t buffer[sizeof(Packet)];
                        memcpy(buffer, &responsePacket, sizeof(Packet));
                        LoRa.beginPacket();
                        for (int i = 0; i < sizeof(Packet); ++i) {
                            LoRa.write(buffer[i]);
                        }
                        LoRa.endPacket();
                        delay(2000); // Wait for response 2 seconds
                        Serial.println("send own key");

                        // mix keys
                        mix_key(receivedPublicKey);
                        delay(5000);
                        Serial.println("mixed keys");
                        hash_key();
                        delay(5000);
                        Serial.println("hashed keys");
//                        break;
//                    }
                    }
                }
            }
        }
    }

    // third step
    // receive encrypted cipherText
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
            //convert from cipherText.toString().c_str()) to uint8_t
            uint8_t gotText[58];
            for (int i = 0; i < 58; i++) {
                gotText[i] = receivedPacket.message[i];
            }
            //decrypt
            char* decrypted = decrypt_message(gotText);
            Serial.println(decrypted);

            char plainText[] = "Hello, AES256!";
            // check if decrypted message is correct
            if (strcmp(decrypted, plainText) == 0) {
                Serial.println("Decrypted message is correct");
            } else {
                Serial.println("Decrypted message is incorrect");
            }
        }
        Serial.println("received hello");

        // MAYBE ONCE I WILL MAKE IT
        //send inform message
        // type of measurements and
//        Packet pairingPacket;
//        uint8_t* cipherText = encode_message();
//
//        createPacket(pairingPacket, HEADER_ENCRYPTED_HELLO, receivedPacket.addressFrom, SENSOR_ADDRESS, "Hello");
//        uint8_t buffer[sizeof(Packet)];
//        memcpy(buffer, &pairingPacket, sizeof(Packet));
//        LoRa.beginPacket();
//        for (int i = 0; i < sizeof(Packet); ++i) {
//            LoRa.write(buffer[i]);
//        }
//        LoRa.endPacket();
//        delay(2000); // Wait for response 2 seconds
    }
    // write to EEPROM
//    Serial.println("started writing eeprom");
//    EEPROM.write(SERVER_ADDRESS, serverAddress >> 8);
//    EEPROM.write(SERVER_ADDRESS + 1, serverAddress & 0xFF);
//
//    for (int i = 0; i < 32; i++) {
//        EEPROM.write(SECRET_HASH_ADDRESS + i, hashedKey[i]);
//    }
//
//    EEPROM.write(IS_PAIRED_ADDRESS, Paired);
//    Serial.println("ended writing eeprom");
//    Serial.println("Device paired successfully.");
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
    }
//    } else {
//        pinMode(gpioPin, INPUT);
//        int state = digitalRead(gpioPin);
//        for (int i = 0; i < 100; ++i) {
//            if (state == HIGH) {
//                Serial.println("GPIO1 is HIGH. Read the address");
//            } else {
//                break;
//            }
//            delay(30);
//            if (i == 99) {
//                handlePairing();
//            }
//        }
//
        readEEPROM();
//    }
}

void loop() {
    Packet packet;
    // encrypt message
    char message[] = "Hello, AES256!";
    uint8_t* cipherText = encode_message(message);
    char* cipherTextChar = reinterpret_cast<char*>(cipherText);
    createPacket(packet, HEADER_MESSAGE, serverAddress, SENSOR_ADDRESS, cipherTextChar);
    LoRa.beginPacket();
    LoRa.write((uint8_t*)&packet, sizeof(Packet));
    LoRa.endPacket();
    delay(1000); // Send message every second
//  Packet packet;
//  Serial.println("sending");
//  createPacket(packet, HEADER_MESSAGE, 0xFFFF, SERVER_ADDRESS, "Hello, world!");
//  uint8_t buffer[sizeof(Packet)];
//  memcpy(buffer, &packet, sizeof(Packet));
//  LoRa.beginPacket();
//  for (int i = 0; i < sizeof(Packet); ++i) {
//      LoRa.write(buffer[i]);
//  }
//  LoRa.endPacket();
//  Serial.println("sent");
//  delay(2000);
}
