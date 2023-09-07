//This example code is in the Public Domain (or CC0 licensed, at your option.)
//By Evandro Copercini - 2018
//
//This example creates a bridge between Serial and Classical Bluetooth (SPP)
//and also demonstrate that SerialBT have the same functionalities of a normal Serial

#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;


#define TOTAL_FLOORS 3
#define FIRST_FLOOR 1
#define SECOND_FLOOR 2
#define THIRD_FLOOR 3

#define UP 1
#define DOWN 2
#define STOP 0

#define WHERE_ARE_YOU 'w'
#define WHAT_STATUS 's'
#define FIRST_FLOOR_PRESS '1'
#define SECOND_FLOOR_PRESS '2'
#define THIRD_FLOOR_PRESS '3'

byte current_pos = SECOND_FLOOR;
byte current_status = STOP;

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ONGNOI"); //Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");
}
char to_char(byte data){
  return data+48;
}

void loop() {
  if (SerialBT.available()) {
    byte data = SerialBT.read();
    if(data == WHERE_ARE_YOU){
      Serial.write("WHERE_ARE_YOU CMD ");
      SerialBT.write(to_char(current_pos));
    }
    else if(data == WHAT_STATUS){
      Serial.write("WHAT_STATUS CMD ");
      SerialBT.write(to_char(current_status));
    }
    else if(data == FIRST_FLOOR_PRESS){
      Serial.write("FIRST_FLOOR CMD ");
      if(current_pos == FIRST_FLOOR){
        Serial.write("YOU ARE IN FIRST_FLOOR ALREADY");
        
      }
      else if(current_pos == SECOND_FLOOR){
        Serial.write("MOVING DOWN");
      }
      else if(current_pos == THIRD_FLOOR){
        Serial.write("MOVING DOWN");
      }
      
    }
    else{
      Serial.write(data);
    }
    
  }
  
  
}