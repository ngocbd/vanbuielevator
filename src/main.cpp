//This example code is in the Public Domain (or CC0 licensed, at your option.)
//By Evandro Copercini - 2018
//
//This example creates a bridge between Serial and Classical Bluetooth (SPP)
//and also demonstrate that SerialBT have the same functionalities of a normal Serial

#include "BluetoothSerial.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>


#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;
BLEDevice BLE;


BLEServer* pServer;
BLEService* pService;
BLECharacteristic* pCharacteristic;



#define TOTAL_FLOORS 3
#define FIRST_FLOOR 1
#define SECOND_FLOOR 2
#define THIRD_FLOOR 3

#define UP 1
#define DOWN 2
#define STOP 0

#define WHERE_ARE_YOU 'w'
#define IDLE_TIME 'i' // IDLE TIME
#define WHAT_STATUS 's' // ENEGINE UP, DOWN, STOP status
#define FIRST_FLOOR_PRESS '1'
#define SECOND_FLOOR_PRESS '2'
#define THIRD_FLOOR_PRESS '3' 
#define TRIGGER_STATUS 't' // TRIGGER STATUS


#define FORCE_STOP 'f'

#define ENGINE_UP_PIN 23
#define ENGINE_DOWN_PIN 22


#define TRIGGER_FIRST_FLOOR_DOWN 25
#define TRIGGER_SECOND_FLOOR_DOWN 26
#define TRIGGER_THIRD_FLOOR_DOWN 27

#define MAX_IDLE_TIME 10000000 // 5 minutes I guess so

byte current_pos = SECOND_FLOOR;
byte current_status = STOP;
byte target_floor = 0;


void setup() {
  Serial.begin(115200);
  // setup bluetooth
  SerialBT.begin("ONGNOII1"); //Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");

  // setup pins mode for engine
  pinMode(ENGINE_UP_PIN, OUTPUT);
  pinMode(ENGINE_DOWN_PIN, OUTPUT);

  // setup pins mode for trigger
  pinMode(TRIGGER_FIRST_FLOOR_DOWN, INPUT_PULLDOWN );
  pinMode(TRIGGER_SECOND_FLOOR_DOWN, INPUT_PULLDOWN );
  pinMode(TRIGGER_THIRD_FLOOR_DOWN, INPUT_PULLDOWN );


   // Khởi tạo BLE
   BLEDevice::init("ONGNOI"); // Đặt tên của thiết bị

  // Tạo một BLE Server
  pServer = BLEDevice::createServer();
 

  // Tạo một BLE Service
  pService = pServer->createService(BLEUUID("00001234-0000-1000-8000-00805f9b34fb")); // Service Generic Access

  // Tạo một BLE Characteristic và thêm vào Service
  pCharacteristic = pService->createCharacteristic(
      BLEUUID("00001001-0000-1000-8000-00805f9b34fb"), // Characteristic Device Name
      BLECharacteristic::PROPERTY_READ |
      BLECharacteristic::PROPERTY_WRITE
  );


  // Thêm Characteristic vào Service
  pService->addCharacteristic(pCharacteristic);

  // Đăng ký Service với Server
  pService->start();

  // Bắt đầu quảng bá tên
  pServer->getAdvertising()->start();
}
  

char to_char(byte data) {
  return data+48;
} 

int forceStop(){
  if(current_status == STOP) {
    return 0;
  }
  digitalWrite(ENGINE_UP_PIN, LOW);
  digitalWrite(ENGINE_DOWN_PIN, LOW);
  current_status = STOP;
  return 1;
}
int ideTime = 0;
int moveToFloor(int floor) {
  // Only move if the elevator is stoped
  if(current_pos == floor) {
    current_status = STOP;
    return 0;
  }
  ideTime = 0;
  target_floor = floor;
  if(current_pos < target_floor) {
    digitalWrite(ENGINE_DOWN_PIN, LOW);
    digitalWrite(ENGINE_UP_PIN, HIGH);
    current_status = UP;
    delay(4000);
    if(current_status == UP){
      current_pos = floor;
    }
    return 1;
  }
  else if(current_pos > target_floor) {
    digitalWrite(ENGINE_UP_PIN, LOW);
    digitalWrite(ENGINE_DOWN_PIN, HIGH);
    current_status = DOWN;
    delay(4000);
    if(current_status == DOWN){
      current_pos = floor;
    }
    return 2;
  }
  
  return 0;
}

int gotoSleep() {
  // move to second floor to sleep
  if (current_pos != SECOND_FLOOR) {
    return moveToFloor(SECOND_FLOOR);
    
  }
  return 0;
}

char digitalToString(byte data) {
  if (data == HIGH) {
    return 'H';
  }
  else {
    return 'L';
  }
}

void loop() {
  ideTime++;
  if(ideTime > MAX_IDLE_TIME) {
    ideTime = 0;
    gotoSleep();
  }                     
  if (digitalRead(TRIGGER_FIRST_FLOOR_DOWN) == HIGH)  {
    Serial.write("TRIGGER_FIRST_FLOOR_DOWN");
    current_pos = FIRST_FLOOR;
    
    if((target_floor == FIRST_FLOOR) && (current_status == DOWN)) {
      forceStop();
      
    }
    return;
    
  }

  if (digitalRead(TRIGGER_SECOND_FLOOR_DOWN) == HIGH)  {
    Serial.write("TRIGGER_SECOND_FLOOR_DOWN");
    current_pos = SECOND_FLOOR; 


     if(target_floor == SECOND_FLOOR) {
      forceStop();
      
    }
    return;
  }

  if (digitalRead(TRIGGER_THIRD_FLOOR_DOWN) == HIGH)  {
    Serial.write("TRIGGER_THIRD_FLOOR_DOWN");
    current_pos = THIRD_FLOOR;
    if((target_floor == THIRD_FLOOR) && (current_status == UP)) {
      forceStop();
      
    }
    return; 
  }
  if(pCharacteristic->getValue().length() > 0){
      std::string value = pCharacteristic->getValue();
      char data = (char)value[0];  // Chuyển giá trị Uint8 thành ký tự ASCII
      if (data == TRIGGER_STATUS) {
      uint8_t triggerValues[3] = {
        static_cast<uint8_t>(digitalToString(digitalRead(TRIGGER_FIRST_FLOOR_DOWN))),
        static_cast<uint8_t>(digitalToString(digitalRead(TRIGGER_SECOND_FLOOR_DOWN))),
        static_cast<uint8_t>(digitalToString(digitalRead(TRIGGER_THIRD_FLOOR_DOWN)))
      };
      pCharacteristic->setValue(triggerValues, sizeof(triggerValues));
      Serial.write(digitalToString(digitalRead(TRIGGER_FIRST_FLOOR_DOWN)));
      Serial.write(digitalToString(digitalRead(TRIGGER_SECOND_FLOOR_DOWN)));
      Serial.write(digitalToString(digitalRead(TRIGGER_THIRD_FLOOR_DOWN)));
    }
    else if(data == IDLE_TIME) {
      Serial.write("IDLE_TIME CMD ");
      uint8_t ideTimeByte = static_cast<uint8_t>(ideTime);
      pCharacteristic->setValue(&ideTimeByte, sizeof(ideTimeByte));
      Serial.println();
      
    }
    else if(data == FORCE_STOP) {
      Serial.write("FORCE_STOP CMD ");
      forceStop();
    }
    else if(data == FIRST_FLOOR_PRESS) {
      Serial.write("FIRST_FLOOR_PRESS CMD ");
      moveToFloor(FIRST_FLOOR);
      
      
    }
    else if(data == SECOND_FLOOR_PRESS) {
      Serial.write("SECOND_FLOOR_PRESS CMD ");
      moveToFloor(SECOND_FLOOR);
      
    }
    else if(data == THIRD_FLOOR_PRESS) {
      Serial.write("THIRD_FLOOR_PRESS CMD ");
      moveToFloor(THIRD_FLOOR);
      
    }
    else if(data == WHERE_ARE_YOU) {
      Serial.write("WHERE_ARE_YOU CMD ");
      Serial.write(data);
      pCharacteristic->setValue(&current_pos, sizeof(current_pos));
      Serial.println();
      
    }
    else if(data == WHAT_STATUS) {
      Serial.write("WHAT_STATUS CMD ");
      Serial.write(data);
      pCharacteristic->setValue(&current_status, sizeof(current_status));
      Serial.println();
    }
    
  }
        
  if (SerialBT.available()) {

    /* Logic for stop elevator when button trigged*/

    byte data = SerialBT.read();
    if (data == TRIGGER_STATUS) {
      SerialBT.write(digitalToString(digitalRead(TRIGGER_FIRST_FLOOR_DOWN)));
      SerialBT.write(digitalToString(digitalRead(TRIGGER_SECOND_FLOOR_DOWN)));
      SerialBT.write(digitalToString(digitalRead(TRIGGER_THIRD_FLOOR_DOWN)));
    }
    else if(data == IDLE_TIME) {
      Serial.write("IDLE_TIME CMD ");
      SerialBT.println(String(ideTime));
      
    }
    else if(data == WHERE_ARE_YOU) {
      Serial.write("WHERE_ARE_YOU CMD ");
      SerialBT.write(to_char(current_pos));
    }
    else if(data == WHAT_STATUS) {
      Serial.write("WHAT_STATUS CMD ");
      SerialBT.write(to_char(current_status));
    }
    else if(data == FORCE_STOP) {
      Serial.write("FORCE_STOP CMD ");
      forceStop();
    }
    else if(data == FIRST_FLOOR_PRESS) {
      Serial.write("FIRST_FLOOR_PRESS CMD ");
      moveToFloor(FIRST_FLOOR);
      
    }
    else if(data == SECOND_FLOOR_PRESS) {
      Serial.write("SECOND_FLOOR_PRESS CMD ");
      moveToFloor(SECOND_FLOOR);
    }
    else if(data == THIRD_FLOOR_PRESS) {
      Serial.write("THIRD_FLOOR_PRESS CMD ");
      moveToFloor(THIRD_FLOOR);
    }
    else{
      Serial.write(data);
    }
    
  }
  
  
}