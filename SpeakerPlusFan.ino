//현재 문제점 선풍기가 켜지면 안멈춘다.
//df mini가 뜨거워진다. 
#include "Arduino.h"
#include "DFRobotDFPlayerMini.h"

#if (defined(ARDUINO_AVR_UNO) || defined(ARDUINO_AVR_NANO) || defined(ESP8266))   
#include <SoftwareSerial.h>
SoftwareSerial softSerial(/*rx =*/2, /*tx =*/3); 
#define FPSerial softSerial
#else
#define FPSerial Serial1
#endif

DFRobotDFPlayerMini myDFPlayer;
void printDetail(uint8_t type, int value);

int motorPin1 = 4;  
int motorPin2 = 5;  

void setup()
{
#if (defined ESP32)
  FPSerial.begin(9600, SERIAL_8N1, /*rx =*/D3, /*tx =*/D2);
#else
  FPSerial.begin(9600);
#endif

  Serial.begin(115200);  

  Serial.println();
  Serial.println(F("DFRobot DFPlayer Mini Demo"));
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));

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

  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);

  randomSeed(analogRead(0)); 
}

void loop()
{
  int randomNumber = random(1, 7); 
  Serial.print(F("Random number: "));
  Serial.println(randomNumber);

  switch (randomNumber) {
    case 1:
      myDFPlayer.play(1); 
      setMotorSpeed(0);  // 선풍기 정지
      break;
    case 2:
      myDFPlayer.play(2); 
      setMotorSpeed(0);
      break;
    case 3:
      myDFPlayer.play(3); 
      setMotorSpeed(0);
      break;
    case 4:
      myDFPlayer.play(4); 
      setMotorSpeed(0);
      break;
    case 5:
      myDFPlayer.play(5); 
      setMotorSpeed(255); // 선풍기 동작
      break;
    case 6:
      myDFPlayer.play(6); 
      setMotorSpeed(0);
      break;
  }

  delay(5000); 

  if (myDFPlayer.available()) {
    printDetail(myDFPlayer.readType(), myDFPlayer.read()); 
  }
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

void setMotorSpeed(int speed) {
  if (speed == 0) {
    digitalWrite(motorPin1, LOW);
    digitalWrite(motorPin2, LOW);
  } else {
    analogWrite(motorPin1, speed);
    digitalWrite(motorPin2, LOW);
  }
}
