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
#include <P521.h>
#include <SHA3.h>
#include <AES.h>
#include <base64.hpp>

bool firstRun = true;
String inString = "";   // string to hold incoming charaters
String MyMessage = "";  // Holds the complete message
uint8_t privateKey1[66];
uint8_t publicKey1[132];

uint8_t privateKey2[66];
uint8_t publicKey2[132];

uint8_t hashedKey[32];

uint8_t encryptedData[32];
uint8_t decryptedData[32];
uint8_t plainText[] = "Hello, AES256!";
uint8_t cipherText[sizeof(plainText)];
uint8_t decrypted[sizeof(plainText)];
AES256 cipherblock;
SHA3_256 sha3;

void generate_keys1() {
  Serial.println("start generating keys for user1");
  P521::dh1(publicKey1, privateKey1);
  Serial.println("generated the key");
  Serial.println("publicKey1: ");
  //base64_buffer[0] = 0;
  //base64_length = encode_base64(publicKey1, 132, base64_buffer);
  //Serial.println(base64_length);
  //Serial.println((char *) base64_buffer);
  /*for(uint8_t elem: publicKey1){
    Serial.print(elem, HEX);
  }
  Serial.print("\n");
  */
  Serial.println("privateKey1:");
  /*for(uint8_t elem: privateKey1){
    Serial.print(elem, HEX);
  }*/
  Serial.print("\n");
  //base64_length = encode_base64(privateKey1, 66, base64);
  //Serial.println((char *) base64);

  //sign_key();
}

void generate_keys2() {
  Serial.println("start generating keys for user2");
  P521::dh1(publicKey2, privateKey2);
  Serial.println("generated the key");

  Serial.println("publicKey2: ");
  /*for(uint8_t elem: publicKey2){
    Serial.print(elem, HEX);
  }*/
  Serial.print("\n");
  //base64_length = encode_base64(publicKey2, 132, base64);
  //Serial.println(base64_length);
  //Serial.println((char *) base64);
  Serial.println("privateKey2:");
  /*for(uint8_t elem: privateKey2){
    Serial.print(elem, HEX);
  }*/
  Serial.print("\n");
  //base64_length = encode_base64(privateKey2, 66, base64);
  //Serial.println((char *) base64);
  //mix_key1();


  //! !!!!!! IF YOU COMMENT THIS IT WILL PRINT KEYS
  const uint8_t received_key = publicKey2;
  Serial.println("derive shared key for user1");
  if (!P521::dh2(received_key, privateKey1)){
    Serial.println("Couldn't derive shared key");
  };
  Serial.println("derived the key");

  Serial.println("sharedKey1: ");
  /*for(uint8_t elem: privateKey1){
    Serial.print(elem, HEX);
  }*/
}

void mix_key1() {
  //sign()
  Serial.println("derive shared key for user1");
  if(!P521::dh2(publicKey2, privateKey1)){
    Serial.println("Couldn't derive shared key");
  };
  Serial.println("derived the key");

  Serial.println("sharedKey1: ");
  for (uint8_t elem : privateKey1) {
    Serial.print(elem, HEX);
  }
  //base64_length = encode_base64(privateKey1, 66, base64_buffer);
  //Serial.println((char *) base64);
}


void mix_key2() {
  //sign()
  Serial.println("derive shared key for user2");
  P521::dh2(publicKey1, privateKey2);
  Serial.println("derived the key");

  Serial.println("sharedKey2: ");
  for (uint8_t elem : privateKey2) {
    Serial.print(elem, HEX);
  }
  //base64_length= encode_base64(privateKey2, 66, base64);
  //Serial.println((char *) base64);
}

/*void hash_key(){
  sha3.update(privateKey1, 66);
  sha3.finalize(hashedKey);
  sha3.reset();
}

void encode_message(){
  cipherblock.setKey(hashedKey, 32);
  cipherblock.encryptBlock(cipherText, plainText);
}

void decrypt_message(){
  cipherblock.decrypt(decrypted, cipherText);
}*/

void setup() {
  Serial.begin(9600);
  /*LoRa.setPins(10, 8, 9);
  while (!Serial);
  Serial.println("LoRa Receiver");
  if (!LoRa.begin(433E6)) { // or 915E6
      Serial.println("Starting LoRa failed!");
    while (1);
  }*/

  //mix_key1();
  //mix_key2();
}

void loop() {
  Serial.println("something");
  generate_keys1();
  generate_keys2();
  /*
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
  }  */
}
