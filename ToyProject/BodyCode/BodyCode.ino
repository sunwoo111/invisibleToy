
/**************************************************************************/
/*!
    @file     readMifare.pde
    @author   Adafruit Industries
  @license  BSD (see license.txt)

    This example will wait for any ISO14443A card or tag, and
    depending on the size of the UID will attempt to read from it.

    If the card has a 4-byte UID it is probably a Mifare
    Classic card, and the following steps are taken:

    - Authenticate block 4 (the first block of Sector 1) using
      the default KEYA of 0XFF 0XFF 0XFF 0XFF 0XFF 0XFF
    - If authentication succeeds, we can then read any of the
      4 blocks in that sector (though only block 4 is read here)

    If the card has a 7-byte UID it is probably a Mifare
    Ultralight card, and the 4 byte pages can be read directly.
    Page 4 is read by default since this is the first 'general-
    purpose' page on the tags.


This is an example sketch for the Adafruit PN532 NFC/RFID breakout boards
This library works with the Adafruit NFC breakout
  ----> https://www.adafruit.com/products/364

Check out the links above for our tutorials and wiring diagrams
These chips use SPI or I2C to communicate.

Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

*/
/**************************************************************************/
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include <VibrationMotor.h>
#include "Arduino.h"
#include "DFRobotDFPlayerMini.h"

#if (defined(ARDUINO_AVR_UNO) || defined(ARDUINO_AVR_NANO) || defined(ESP8266))   
#include <SoftwareSerial.h>
SoftwareSerial softSerial(/*rx =*/2, /*tx =*/3); 
#define FPSerial softSerial
#else
#define FPSerial Serial1
#endif
#define bodyColor  2    // red : 1, blue = 2, yellow = 3

// If using the breakout with SPI, define the pins for SPI communication.
#define PN532_SCK  (13)
#define PN532_MOSI (11)
#define PN532_SS   (10)
#define PN532_MISO (12)

#define vibrationMotorPin        6
#define FanMotorPin1              4
#define FanMotorPin2             5

// If using the breakout or shield with I2C, define just the pins connected
// to the IRQ and reset lines.  Use the values below (2, 3) for the shield!
#define PN532_IRQ   (2)
#define PN532_RESET (3)  // Not connected by default on the NFC Shield

String lastTag = "";  
// Uncomment just _one_ line below depending on how your breakout or shield
// is connected to the Arduino:

// Use this line for a breakout with a software SPI connection (recommended):
Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

VibrationMotor motor(vibrationMotorPin);


DFRobotDFPlayerMini myDFPlayer;
void printDetail(uint8_t type, int value);
// Use this line for a breakout with a hardware SPI connection.  Note that
// the PN532 SCK, MOSI, and MISO pins need to be connected to the Arduino's
// hardware SPI SCK, MOSI, and MISO pins.  On an Arduino Uno these are
// SCK = 13, MOSI = 11, MISO = 12.  The SS line can be any digital IO pin.
//Adafruit_PN532 nfc(PN532_SS);

// Or use this line for a breakout or shield with an I2C connection:
//Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

// Or use hardware Serial:
//Adafruit_PN532 nfc(PN532_RESET, &Serial1);



void setup(void) {
  Serial.begin(9600);
  while (!Serial) delay(10); // for Leonardo/Micro/Zero

  Serial.println("Hello!");


  nfc.begin();

  nfc.setPassiveActivationRetries(0x10);
  nfc.SAMConfig();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX);
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC);
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);

  Serial.println("Waiting for an ISO14443A Card ...");

  #if (defined ESP32)
  FPSerial.begin(9600, SERIAL_8N1, /*rx =*/D3, /*tx =*/D2);
#else
  FPSerial.begin(9600);
#endif

  if (!myDFPlayer.begin(FPSerial, /*isACK = */true, /*doReset = */true)) {  
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
   while(true){
      delay(0); 
    }
  }
  Serial.println(F("DFPlayer Mini online."));

  myDFPlayer.volume(30);  

}


void loop(void) {
  
byte redBlock[]    = {
        0x0A, 0x11, 0x00, 0x00, //  1,  2,   3,  4,
        0xff, 0xff, 0xff, 0xff, //  5,  6,   7,  8,
        0xff, 0xff, 0xff, 0xff, //  9, 10, 255, 11,
        0xff, 0xff, 0xff, 0xff  // 12, 13, 14, 15
    };
    byte blueBlock[]    = {
        0x0B, 0x00, 0x11, 0x00, //  1,  2,   3,  4,
        0xff, 0xff, 0xff, 0xff, //  5,  6,   7,  8,
        0xff, 0xff, 0xff, 0xff, //  9, 10, 255, 11,
        0xff, 0xff, 0xff, 0xff  // 12, 13, 14, 15
    };
    byte yellowBlock[]    = {
        0x0C, 0x00, 0x00, 0x11, //  1,  2,   3,  4,
        0xff, 0xff, 0xff, 0xff, //  5,  6,   7,  8,
        0xff, 0xff, 0xff, 0xff, //  9, 10, 255, 11,
        0xff, 0xff, 0xff, 0xff  // 12, 13, 14, 15
    };

  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  
  String currentTag = "";
  int isRed = 1; int isYellow = 1; int isBlue = 1;
  int cardColor = 0;

  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
    Serial.println(lastTag);
  
  if (success) {
    // Display some basic information about the card
    Serial.println("Found an ISO14443A card");
    Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(uid, uidLength);
    Serial.println("");

    for(int i=0; i<uidLength; i++){
      currentTag += String(uid[i], HEX);
    }
    Serial.println(currentTag);

    if(lastTag == currentTag){
      Serial.print("same tag!");
      delay(1000);
      return;
    }

    lastTag = currentTag;

    if (uidLength == 4)
    {
      // We probably have a Mifare Classic card ...
      Serial.println("Seems to be a Mifare Classic card (4 byte UID)");

      // Now we need to try to authenticate it for read/write access
      // Try with the factory default KeyA: 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF
      Serial.println("Trying to authenticate block 4 with default KEYA value");
      uint8_t keya[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

    // Start with block 4 (the first block of sector 1) since sector 0
    // contains the manufacturer data and it's probably better just
    // to leave it alone unless you know what you're doing
      success = nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 0, keya);

      if (success)
      {
        Serial.println("Sector 1 (Blocks 4..7) has been authenticated");
        byte data[16];

        // If you want to write something to block 4 to test with, uncomment
    // the following line and this text should be read back in a minute
        //memcpy(data, (const uint8_t[]){ 'a', 'd', 'a', 'f', 'r', 'u', 'i', 't', '.', 'c', 'o', 'm', 0, 0, 0, 0 }, sizeof data);
        // success = nfc.mifareclassic_WriteDataBlock (4, data);

        // Try to read the contents of block 4
        success = nfc.mifareclassic_ReadDataBlock(4, data);

        if (success)
        {
          // Data seems to have been read ... spit it out
          Serial.println("Reading Block 4:");
          nfc.PrintHexChar(data, 16);
         
          for(int i=0; i< 4; i++){
            if(redBlock[i] != data[i]){
              isRed = 0;
              //Serial.print("it is not same");
            }
            if(yellowBlock[i] != data[i]){
              isYellow = 0;
              //Serial.print("it is not same");
            }
            if(blueBlock[i] != data[i]){
              isBlue = 0;
              //Serial.print("it is not same");
            }
          }
          if(isRed && !isBlue && !isYellow){
            cardColor = 1; //red
          }
          else if(!isRed && isBlue && !isYellow ){

            cardColor = 2; //blue
          }
          else if(!isRed && !isBlue && isYellow ){
          
            cardColor = 3; //yellow
          }
          else{
            Serial.print("it is wrong in bit of color"); 
          }
        }
        else
        {
          Serial.println("Ooops ... unable to read the requested block.  Try another key?");
        }
      }
      else
      {
        Serial.println("Ooops ... authentication failed: Try another key?");
      }
    }

    if (uidLength == 7)
    {
      // We probably have a Mifare Ultralight card ...
      Serial.println("Seems to be a Mifare Ultralight tag (7 byte UID)");

      // Try to read the first general-purpose user page (#4)
      Serial.println("Reading page 4");
      byte data[32];
      success = nfc.mifareultralight_ReadPage (4, data);
      if (success)
      {
        // Data seems to have been read ... spit it out
        nfc.PrintHexChar(data, 4);
        Serial.println("");
         for(int i=0; i< 4; i++){
            if(redBlock[i] != data[i]){
              isRed = 0;
              //Serial.print("it is not same");
            }
            if(yellowBlock[i] != data[i]){
              isYellow = 0;
              //Serial.print("it is not same");
            }
            if(blueBlock[i] != data[i]){
              isBlue = 0;
              //Serial.print("it is not same");
            }
          }
          if(isRed && !isBlue && !isYellow){
            cardColor = 1; //red
          }
          else if(!isRed && isBlue && !isYellow ){

            cardColor = 2; //blue
          }
          else if(!isRed && !isBlue && isYellow ){
          
            cardColor = 3; //yellow
          }
          else{
            Serial.print("it is wrong in bit of color"); return;
          }
        // Wait a bit before reading the card again
        delay(1000);
      }
      else
      {
        Serial.println("Ooops ... unable to read the requested page!?");
      }
    }
  }
  else{
    Serial.println("not tagging");
    lastTag = "";
    delay(1000);
  }

  Serial.print("the card Color is"); Serial.println(cardColor);
  // red : 1, blue : 2, yellow : 3
  // purple = blue + red
  // green = yellow + blue
  // orange = red + yellow
  bool isredAction = ((bodyColor == 1) && (cardColor == 1));
  bool isblueAction = ((bodyColor == 2) && (cardColor == 2));
  bool isyellowAction = ((bodyColor == 3) && (cardColor == 3));
 
  bool ispurpleAction = ((bodyColor == 1) && (cardColor == 2)) || ((bodyColor == 2) && (cardColor == 1));
  bool isgreenAction = ((bodyColor == 2) && (cardColor == 3)) || ((bodyColor == 3) && (cardColor == 2));
  bool isorangeAction = ((bodyColor == 1) && (cardColor == 3)) || ((bodyColor == 3) && (cardColor == 1));
  if(isredAction) redAction();
  else if(isblueAction) blueAction();
  else if(isyellowAction) yellowAction();

  else if(ispurpleAction) purpleAction();
  else if(isgreenAction) greenAction();
  else if(isorangeAction) orangeAction();

  else{
    Serial.println("we do not know the color?");
    
  }

  delay(1000);



}

void printDetail(uint8_t type, int value){
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerUSBInserted:
      Serial.println("USB Inserted!");
      break;
    case DFPlayerUSBRemoved:
      Serial.println("USB Removed!");
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

//song number
// 1: red 2: orange 3:yellow
// 4:green 5:blue 6:purple
void redAction(){
  Serial.print("I am red!");
  myDFPlayer.play(1);
  setMotorSpeed(0);
  for(int i=0; i<10; i++){
     motor.on();
     delay(500);
     motor.off();
    delay(500);
  }
  motor.off();
}
void blueAction(){
  myDFPlayer.play(5);
  Serial.print("I am blue!");
  setMotorSpeed(255);
  for(int i=0; i<30; i++){
    motor.on();
    delay(100);
    motor.off();
    delay(100);
  }
  setMotorSpeed(0);
  motor.off();
}
void yellowAction(){
  Serial.print("I am yellow!");
  setMotorSpeed(0);
  myDFPlayer.play(3); 
  for(int i=0; i<16; i++){
    motor.on();
    delay(250);
     motor.off();
    delay(250);
  }
  motor.off();
}


void orangeAction(){
  Serial.print("I am orange!");
  setMotorSpeed(0);
  myDFPlayer.play(2);
  for(int i=0; i<15; i++){
    motor.on();
    delay(400);
    motor.on(180);
    delay(400);
  }
  motor.off();
}
void greenAction(){
  myDFPlayer.play(4);
  setMotorSpeed(0);
  Serial.print("I am green!");
  for(int i=0; i<30; i++){
    motor.on();
    delay(220);
    motor.on(180);
    delay(220);
  }
  motor.off();
}
void purpleAction(){
  Serial.print("I am purple!");
  myDFPlayer.play(6); 
  setMotorSpeed(0);
  for(int i=0; i<30; i++){
    motor.on();
    delay(120);
    motor.on(180);
    delay(120);
    
  }
  motor.off();
}

void setMotorSpeed(int speed) {
  if (speed == 0) {
    digitalWrite(FanMotorPin1, LOW);
    digitalWrite(FanMotorPin2, LOW);
  } else {
    analogWrite(FanMotorPin1, speed);
    digitalWrite(FanMotorPin2, LOW);
  }
}
