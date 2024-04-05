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

String inString = "";    // string to hold incoming charaters
String MyMessage = ""; // Holds the complete message
bool isPaired;
int gpioPin = 1;
unsigned long previousMillis = 0; // Variable to store the previous time
const long interval = 30000; // Interval in milliseconds (30 seconds)

void setup() {
    Serial.begin(9600);
    LoRa.setPins(10, 8, 9);
    while (!Serial);
    Serial.println("LoRa Receiver");
    if (!LoRa.begin(433E6)) { // or 915E6
        Serial.println("Starting LoRa failed!");
        while (1);
    }
    pinMode(gpioPin, INPUT);
    int state = digitalRead(gpioPin);
    if (isPaired){
        for(int i = 0; i < 100; ++i){
            if (state == HIGH) {
                if (millis() - previousMillis >= interval) {
                    previousMillis = millis();
                } else{
                    return;
                }
            }
            pairing();
        } else {
            pairing();
        }

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

    }

