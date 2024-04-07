/* 
 *  Receiver Side Code
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
String inString = "";
String MyMessage = "";

int gpioPin = 1;

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

void pairing(){
    // ...
    uint16_t serverAddress = 1234;  // get from protocol, Volodya part
    EEPROM.write(SERVER_ADDRESS_ADDRESS, serverAddress >> 8);
    EEPROM.write(SERVER_ADDRESS_ADDRESS + 1, serverAddress & 0xFF);

    byte secretHashBytes[32]; // get from protocol, Volodya part
    for (int i = 0; i < 32; i++) {
        secretHashBytes[i] = i;
        EEPROM.write(SECRET_HASH_ADDRESS + i, secretHashBytes[i]);
    }

    Serial.println("Data written to EEPROM");
}

void setup() {
    Serial.begin(9600);
    LoRa.setPins(10, 8, 9);
    while (!Serial);
    Serial.println("LoRa Receiver");
    if (!LoRa.begin(433E6)) { // or 915E6
        Serial.println("Starting LoRa failed!");
        while (1);
    }

    isPaired = EEPROM.read(IS_PAIRED_ADDRESS);

    if (isPaired == notPaired){
        Serial.println("I am ready to pairing!");
        pairing();
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
                pairing();
            }
        }

        readEEPROM();
    }

    void loop() {

        // try to parse packet
        int packetSize = LoRa.parsePacket();
//  Serial.println(packetSize);
        if (packetSize) {
            // read packet
            while (LoRa.available())
            {
                int inChar = LoRa.read();
                inString += (char)inChar;
                MyMessage = inString;

            }
            inString = "";
            LoRa.packetRssi();
            Serial.println(MyMessage);
        }

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

    }
