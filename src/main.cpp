// This example code is in the Public Domain (or CC0 licensed, at your option.)
// By Evandro Copercini - 2018
//
// This example creates a bridge between Serial and Classical Bluetooth (SPP)
// and also demonstrate that SerialBT have the same functionalities of a normal Serial

#include "BluetoothSerial.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <ezButton.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;
BLEDevice BLE;

BLEServer *pServer;
BLEService *pService;
BLECharacteristic *pCharacteristic;

#define TOTAL_FLOORS 3
#define FIRST_FLOOR 1
#define SECOND_FLOOR 2
#define THIRD_FLOOR 3

#define UP 1
#define DOWN 2
#define STOP 0

#define WHERE_ARE_YOU 'w'
#define IDLE_TIME 'i'   // IDLE TIME
#define WHAT_STATUS 's' // ENEGINE UP, DOWN, STOP status
#define FIRST_FLOOR_PRESS '1'
#define SECOND_FLOOR_PRESS '2'
#define THIRD_FLOOR_PRESS '3'
#define TRIGGER_STATUS 't' // TRIGGER STATUS

#define FORCE_STOP 'f'

#define ENGINE_UP_PIN 23
#define ENGINE_DOWN_PIN 22

#define ONEST_FLOOR 18
#define TWOND_FLOOR 19
#define THREE_FLOOR 21

#define TRIGGER_FIRST_FLOOR_DOWN 25
#define TRIGGER_SECOND_FLOOR_DOWN 26
#define TRIGGER_THIRD_FLOOR_DOWN 27

#define DOOR_CLOSE_TRIGGER 33

#define BOTTOM_STOP_TRIGGER 32

#define ELEVATOR_BUZZER 32
#define BUZZER_FREQUENCY 2000
#define BUZZER_DURATION 100

#define DEBOUNCE_TIME 50 // the debounce time in millisecond, increase this time if it still chatters
byte current_pos = FIRST_FLOOR;
byte current_status = STOP;
byte target_floor = 0;
byte is_door_close = 0;
byte is_pausing = 0; // elevator is pausing

ezButton firstFloorButton(ONEST_FLOOR);
ezButton secondFloorButton(TWOND_FLOOR);
ezButton thirdFloorButton(THREE_FLOOR);
ezButton triggerFirstFloor(TRIGGER_FIRST_FLOOR_DOWN);
ezButton triggerSecondFloor(TRIGGER_SECOND_FLOOR_DOWN);
ezButton triggerThirdFloor(TRIGGER_THIRD_FLOOR_DOWN);
ezButton doorCloseTrigger(DOOR_CLOSE_TRIGGER);
ezButton bottomStopTrigger(BOTTOM_STOP_TRIGGER);

void setup()
{
  Serial.begin(115200);
  // setup bluetooth
  SerialBT.begin("ONGNOII1"); // Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");

  firstFloorButton.setDebounceTime(DEBOUNCE_TIME);
  secondFloorButton.setDebounceTime(DEBOUNCE_TIME);
  thirdFloorButton.setDebounceTime(DEBOUNCE_TIME);
  triggerFirstFloor.setDebounceTime(DEBOUNCE_TIME);
  triggerSecondFloor.setDebounceTime(DEBOUNCE_TIME);
  triggerThirdFloor.setDebounceTime(DEBOUNCE_TIME);
  doorCloseTrigger.setDebounceTime(DEBOUNCE_TIME);
  bottomStopTrigger.setDebounceTime(DEBOUNCE_TIME);

  // setup pins mode for engine
  pinMode(ENGINE_UP_PIN, OUTPUT);
  pinMode(ENGINE_DOWN_PIN, OUTPUT);

  // pin mode for buzzer
  pinMode(ELEVATOR_BUZZER, OUTPUT);
  // turn off buzzer
  digitalWrite(ELEVATOR_BUZZER, HIGH);

  // Initialize BLE
  BLEDevice::init("ONGNOI"); // Set the name of the device

  // Create a BLE Server
  pServer = BLEDevice::createServer();

  // Create a BLE Service
  pService = pServer->createService(BLEUUID("00001234-0000-1000-8000-00805f9b34fb")); // Service Generic Access

  // Create a BLE Characteristic and add it to Service
  pCharacteristic = pService->createCharacteristic(
      BLEUUID("00001001-0000-1000-8000-00805f9b34fb"), // Characteristic Device Name
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE);

  // Add Characteristics to Service
  pService->addCharacteristic(pCharacteristic);

  // Register Service with Server
  pService->start();

  // Start promoting the name
  pServer->getAdvertising()->start();
}

char to_char(byte data)
{
  return data + 48;
}

int forceStop()
{
  if (current_status == STOP)
  {
    return 0;
  }
  digitalWrite(ENGINE_UP_PIN, LOW);
  digitalWrite(ENGINE_DOWN_PIN, LOW);
  current_status = STOP;
  return 1;
}

int moveToFloor(int floor)
{
  // Only move if the elevator is not at the target floor
  if (current_pos == floor)
  {
    // current_status = STOP;
    return 0;
  }
  // Only move if the engine is not moving
  if (current_status != STOP)
  {
    return 0;
  }
  // only move if the door is close
  if (is_door_close == 0)
  {
    return 0;
  }

  target_floor = floor;
  if (current_pos < target_floor)
  {
    digitalWrite(ENGINE_DOWN_PIN, LOW);
    digitalWrite(ENGINE_UP_PIN, HIGH);
    current_status = UP;
    return 1;
  }
  else if (current_pos > target_floor)
  {
    digitalWrite(ENGINE_UP_PIN, LOW);
    digitalWrite(ENGINE_DOWN_PIN, HIGH);
    current_status = DOWN;
    return 2;
  }
  return 0;
}

char digitalToString(byte data)
{
  if (data == HIGH)
  {
    return 'H';
  }
  else
  {
    return 'L';
  }
}

int playTone()
{
  Serial.println("playTone");
  int halfPeriod = 1000000L / (BUZZER_FREQUENCY * 2);                 // Calculate half period in microseconds
  unsigned long cycles = (BUZZER_FREQUENCY * BUZZER_DURATION) / 1000; // Calculate the number of cycles for the given duration

  for (unsigned long i = 0; i < cycles; i++)
  {
    digitalWrite(ELEVATOR_BUZZER, LOW);
    delayMicroseconds(halfPeriod);
    digitalWrite(ELEVATOR_BUZZER, HIGH);
    delayMicroseconds(halfPeriod);
  }
  return 1;
}

void loop()
{
  firstFloorButton.loop();
  secondFloorButton.loop();
  thirdFloorButton.loop();
  triggerFirstFloor.loop();
  triggerSecondFloor.loop();
  triggerThirdFloor.loop();
  doorCloseTrigger.loop();
  bottomStopTrigger.loop();
  // when the trigger at the bottom of the elevator is pressed (meaning some  obstacle is in the way) stop the elevator
  if (bottomStopTrigger.getState() == LOW || doorCloseTrigger.getState() == HIGH)
  {
    digitalWrite(ENGINE_UP_PIN, LOW);
    digitalWrite(ENGINE_DOWN_PIN, LOW);
    is_pausing = 1;
  }
  // continue the elevator when the obstacle is removed and the door is closed
  if (is_pausing == 1 && bottomStopTrigger.getState() == HIGH && doorCloseTrigger.getState() == LOW)
  {
    if (current_status == UP)
    {
      
      delay(1000);
      digitalWrite(ENGINE_UP_PIN, HIGH);
      digitalWrite(ENGINE_DOWN_PIN, LOW);
    }
    if (current_status == DOWN)
    {
      delay(1000);
      digitalWrite(ENGINE_DOWN_PIN, HIGH);
      digitalWrite(ENGINE_UP_PIN, LOW);
    }
    is_pausing = 0;
  }

  if (triggerFirstFloor.isPressed())
  {
    Serial.write("TRIGGER_FIRST_FLOOR_DOWN");
    current_pos = FIRST_FLOOR;

    if ((target_floor == FIRST_FLOOR) && (current_status == DOWN))
    {
      forceStop();
    }
    playTone();
    return;
  }

  if (triggerSecondFloor.isPressed())
  {
    Serial.write("TRIGGER_SECOND_FLOOR_DOWN");
    current_pos = SECOND_FLOOR;

    if (target_floor == SECOND_FLOOR)
    {
      forceStop();
    }
    playTone();
    return;
  }

  if (triggerThirdFloor.isPressed())
  {
    Serial.write("TRIGGER_THIRD_FLOOR_DOWN");
    current_pos = THIRD_FLOOR;
    if ((target_floor == THIRD_FLOOR) && (current_status == UP))
    {
      forceStop();
    }
    playTone();
    return;
  }
  // door status //low because of pull up resistor
  if (doorCloseTrigger.getState() == LOW)
  {
    is_door_close = 1;
  }
  else
  {
    is_door_close = 0;
  }

  if (pCharacteristic->getValue().length() > 0)
  {
    std::string value = pCharacteristic->getValue();
    char data = (char)value[0]; // Chuyển giá trị Uint8 thành ký tự ASCII
    if (data == TRIGGER_STATUS)
    {
      uint8_t triggerValues[3] = {
          static_cast<uint8_t>(digitalToString(digitalRead(TRIGGER_FIRST_FLOOR_DOWN))),
          static_cast<uint8_t>(digitalToString(digitalRead(TRIGGER_SECOND_FLOOR_DOWN))),
          static_cast<uint8_t>(digitalToString(digitalRead(TRIGGER_THIRD_FLOOR_DOWN)))};
      pCharacteristic->setValue(triggerValues, sizeof(triggerValues));
      Serial.write(digitalToString(digitalRead(TRIGGER_FIRST_FLOOR_DOWN)));
      Serial.write(digitalToString(digitalRead(TRIGGER_SECOND_FLOOR_DOWN)));
      Serial.write(digitalToString(digitalRead(TRIGGER_THIRD_FLOOR_DOWN)));
    }
    // else if (data == IDLE_TIME)
    // {
    //   Serial.write("IDLE_TIME CMD ");
    //   uint8_t ideTimeByte = static_cast<uint8_t>(ideTime);
    //   pCharacteristic->setValue(&ideTimeByte, sizeof(ideTimeByte));
    //   Serial.println();
    // }
    else if (data == FORCE_STOP)
    {
      Serial.write("FORCE_STOP CMD ");
      forceStop();
    }
    else if (data == FIRST_FLOOR_PRESS)
    {
      Serial.write("FIRST_FLOOR_PRESS CMD ");
      moveToFloor(FIRST_FLOOR);
    }
    else if (data == SECOND_FLOOR_PRESS)
    {
      Serial.write("SECOND_FLOOR_PRESS CMD ");
      moveToFloor(SECOND_FLOOR);
    }
    else if (data == THIRD_FLOOR_PRESS)
    {
      Serial.write("THIRD_FLOOR_PRESS CMD ");
      moveToFloor(THIRD_FLOOR);
    }
    else if (data == WHERE_ARE_YOU)
    {
      Serial.write("WHERE_ARE_YOU CMD ");
      Serial.write(data);
      pCharacteristic->setValue(&current_pos, sizeof(current_pos));
      Serial.println();
    }
    else if (data == WHAT_STATUS)
    {
      Serial.write("WHAT_STATUS CMD ");
      Serial.write(data);
      pCharacteristic->setValue(&current_status, sizeof(current_status));
      Serial.println();
    }
  }

  if (SerialBT.available())
  {

    /* Logic for stop elevator when button trigged*/

    byte data = SerialBT.read();
    if (data == TRIGGER_STATUS)
    {
      SerialBT.write(digitalToString(triggerFirstFloor.getState()));
      SerialBT.write(digitalToString(triggerSecondFloor.getState()));
      SerialBT.write(digitalToString(triggerThirdFloor.getState()));
    }
    // else if (data == IDLE_TIME)
    // {
    //   Serial.write("IDLE_TIME CMD ");
    //   SerialBT.println(String(ideTime));
    // }
    else if (data == WHERE_ARE_YOU)
    {
      Serial.write("WHERE_ARE_YOU CMD ");
      SerialBT.write(to_char(current_pos));
    }
    else if (data == WHAT_STATUS)
    {
      Serial.write("WHAT_STATUS CMD ");
      SerialBT.write(to_char(current_status));
    }
    else if (data == FORCE_STOP)
    {
      Serial.write("FORCE_STOP CMD ");
      forceStop();
    }
    else if (data == FIRST_FLOOR_PRESS)
    {
      Serial.write("FIRST_FLOOR_PRESS CMD ");
      moveToFloor(FIRST_FLOOR);
    }
    else if (data == SECOND_FLOOR_PRESS)
    {
      Serial.write("SECOND_FLOOR_PRESS CMD ");
      moveToFloor(SECOND_FLOOR);
    }
    else if (data == THIRD_FLOOR_PRESS)
    {
      Serial.write("THIRD_FLOOR_PRESS CMD ");
      moveToFloor(THIRD_FLOOR);
    }
    else
    {
      Serial.write(data);
    }
  }

  if (firstFloorButton.isPressed())
  {
    moveToFloor(FIRST_FLOOR);
    Serial.print("ONEST_FLOOR");
    Serial.println();
    return;
  }
  if (secondFloorButton.isPressed())
  {
    moveToFloor(SECOND_FLOOR);
    Serial.print("TWOND_FLOOR");
    Serial.println();
    return;
  }

  if (thirdFloorButton.isPressed())
  {
    moveToFloor(THIRD_FLOOR);
    Serial.print("THREE_FLOOR");
    Serial.println();
    return;
  }
}