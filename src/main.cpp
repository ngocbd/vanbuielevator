#include "BluetoothSerial.h"
#include <ezButton.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <MqttLogger.h>
#include <string.h>
#include <esp_task_wdt.h>

const char *ssid = "Van Phien";
const char *password = "25122013";
const char *mqtt_server = "103.199.18.51";
static unsigned long lastPingTime = 0;
WiFiClient espClient;
PubSubClient client(espClient);

// default mode is MqttLoggerMode::MqttAndSerialFallback
MqttLogger mqttLogger(client, "ongnoi/thangmay");
// other available modes:
// MqttLogger mqttLogger(client,"mqttlogger/log",MqttLoggerMode::MqttAndSerial);
// MqttLogger mqttLogger(client,"mqttlogger/log",MqttLoggerMode::MqttOnly);
// MqttLogger mqttLogger(client,"mqttlogger/log",MqttLoggerMode::SerialOnly);

// connect to wifi network
void wifiConnect()
{
  mqttLogger.print("Connecting to WiFi: ");
  mqttLogger.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    mqttLogger.print(".");
  }
  mqttLogger.print("WiFi connected: ");
  mqttLogger.println(WiFi.localIP());
}

// establish mqtt client connection
void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    mqttLogger.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP32Logger"))
    {
      // as we have a connection here, this will be the first message published to the mqtt server
      mqttLogger.println("connected.");
    }
    else
    {
      mqttLogger.print("failed, rc=");
      mqttLogger.print(client.state());
      mqttLogger.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

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
#define WHAT_STATUS 's' // ENEGINE UP, DOWN, STOP status
#define FIRST_FLOOR_PRESS '1'
#define SECOND_FLOOR_PRESS '2'
#define THIRD_FLOOR_PRESS '3'
#define TRIGGER_STATUS 't' // TRIGGER STATUS
#define FORCE_STOP 'f'
#define DOOR_STATUS 'd'
#define BOTTOM_STOP_STATUS 'b'

#define ENGINE_UP_PIN 23
#define ENGINE_DOWN_PIN 22

#define ONEST_FLOOR 18
#define TWOND_FLOOR 19
#define THREE_FLOOR 21

#define ONEST_FLOOR_LED_PIN 13
#define TWOND_FLOOR_LED_PIN 12
#define THREE_FLOOR_LED_PIN 14

#define TRIGGER_FIRST_FLOOR_DOWN 25
#define TRIGGER_SECOND_FLOOR_DOWN 26
#define TRIGGER_THIRD_FLOOR_DOWN 27

#define DOOR_CLOSE_TRIGGER 33

#define BOTTOM_STOP_TRIGGER 32

#define DEBOUNCE_TIME 50 // the debounce time in millisecond, increase this time if it still chatters

#define WDT_TIMEOUT 30 // 30 seconds WDT

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
byte isSystemStartLogSent = 0;
void elevatorLog(String message)
{
  Serial.println(message);
  mqttLogger.println(message);
  SerialBT.println(message);
}

void setup()
{
  Serial.begin(115200);
  // setup bluetooth
  SerialBT.begin("ONGNOI");
  Serial.println("The device started, now you can pair it with bluetooth!");

  firstFloorButton.setDebounceTime(DEBOUNCE_TIME);
  secondFloorButton.setDebounceTime(DEBOUNCE_TIME);
  thirdFloorButton.setDebounceTime(DEBOUNCE_TIME);
  triggerFirstFloor.setDebounceTime(DEBOUNCE_TIME);
  triggerSecondFloor.setDebounceTime(DEBOUNCE_TIME);
  triggerThirdFloor.setDebounceTime(DEBOUNCE_TIME);
  doorCloseTrigger.setDebounceTime(DEBOUNCE_TIME);
  bottomStopTrigger.setDebounceTime(DEBOUNCE_TIME);

  pinMode(ONEST_FLOOR_LED_PIN, OUTPUT);
  pinMode(TWOND_FLOOR_LED_PIN, OUTPUT);
  pinMode(THREE_FLOOR_LED_PIN, OUTPUT);

  // setup pins mode for engine
  pinMode(ENGINE_UP_PIN, OUTPUT);
  pinMode(ENGINE_DOWN_PIN, OUTPUT);

  mqttLogger.println("Starting setup..");

  // connect to wifi
  wifiConnect();

  // mqtt client
  client.setServer(mqtt_server, 1883);

  // init wdt
  esp_task_wdt_init(WDT_TIMEOUT, true); // enable panic so ESP32 restarts
  esp_task_wdt_add(NULL);
  Serial.println("System started");
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
  // turn on led
  switch (target_floor)
  {
  case 1:
    digitalWrite(ONEST_FLOOR_LED_PIN, HIGH);
    break;
  case 2:
    digitalWrite(TWOND_FLOOR_LED_PIN, HIGH);
    break;
  case 3:
    digitalWrite(THREE_FLOOR_LED_PIN, HIGH);
    break;

  default:
    break;
  }
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

void elevatorSecurityCheck()
{
  // when the trigger at the bottom of the elevator is pressed (meaning some  obstacle is in the way) stop the elevator
  if (bottomStopTrigger.getState() == HIGH || doorCloseTrigger.getState() == HIGH)
  {
    // elevatorLog("pausing");
    // Serial.println("pausing");
    digitalWrite(ENGINE_UP_PIN, LOW);
    digitalWrite(ENGINE_DOWN_PIN, LOW);
    is_pausing = 1;
  }
  // continue the elevator when the obstacle is removed and the door is closed
  if (is_pausing == 1 && bottomStopTrigger.getState() == LOW && doorCloseTrigger.getState() == LOW)
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
}

void onTriggerFloorDown(int floor)
{
  char logMessage[50]; // Allocate a buffer for the log message
  snprintf(logMessage, sizeof(logMessage), "Trigger at floor %d is pressed", floor);

  elevatorLog(logMessage);
  // only update the current position when the elevator is moving
  if (current_status != STOP)
  {
    current_pos = floor;
  }
  if ((target_floor == floor))
  {
    forceStop();
    // turn off the led
    switch (floor)
    {
    case 1:
      digitalWrite(ONEST_FLOOR_LED_PIN, LOW);
      break;
    case 2:
      digitalWrite(TWOND_FLOOR_LED_PIN, LOW);
      break;
    case 3:
      digitalWrite(THREE_FLOOR_LED_PIN, LOW);
      break;

    default:
      break;
    }
  }

  return;
}

void onElevatorButtonDown(int floor)
{
  moveToFloor(floor);
  char logMessage[50]; // Allocate a buffer for the log message
  snprintf(logMessage, sizeof(logMessage), "Button at floor %d is pressed", floor);
  elevatorLog(logMessage);
  return;
}

void sendBlackBox()
{
  // send the black box data to the mqtt server
  char blackBoxMessage[110]; // Allocate a buffer for the log message
  // build json string
  snprintf(blackBoxMessage, sizeof(blackBoxMessage), "{\"current_pos\": %d, \"current_status\": %d, \"target_floor\": %d, \"is_door_close\": %d, \"is_pausing\": %d}", current_pos, current_status, target_floor, is_door_close, is_pausing);
  mqttLogger.println(blackBoxMessage);
  lastPingTime = millis();
}

void loop()
{
  // reset wdt
  esp_task_wdt_reset();
  firstFloorButton.loop();
  secondFloorButton.loop();
  thirdFloorButton.loop();
  triggerFirstFloor.loop();
  triggerSecondFloor.loop();
  triggerThirdFloor.loop();
  doorCloseTrigger.loop();
  bottomStopTrigger.loop();
  // here the mqtt connection is established
  if (!client.connected())
  {
    reconnect();
  }
  if (!isSystemStartLogSent)
  {
    elevatorLog("System started");
    isSystemStartLogSent = 1;
  }
  elevatorSecurityCheck();
  // physical buttons logic
  //  door status -> low because of pull up resistor
  is_door_close = doorCloseTrigger.getState() == LOW ? 1 : 0;

  if (triggerFirstFloor.isPressed())
    onTriggerFloorDown(FIRST_FLOOR);
  if (triggerSecondFloor.isPressed())
    onTriggerFloorDown(SECOND_FLOOR);
  if (triggerThirdFloor.isPressed())
    onTriggerFloorDown(THIRD_FLOOR);

  if (firstFloorButton.isPressed())
    onElevatorButtonDown(FIRST_FLOOR);
  if (secondFloorButton.isPressed())
    onElevatorButtonDown(SECOND_FLOOR);
  if (thirdFloorButton.isPressed())
    onElevatorButtonDown(THIRD_FLOOR);
  // bluetooth logic

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
    else if (data == WHERE_ARE_YOU)
    {
      elevatorLog("WHERE_ARE_YOU CMD ");
      SerialBT.write(to_char(current_pos));
    }
    else if (data == WHAT_STATUS)
    {
      elevatorLog("WHAT_STATUS CMD ");
      SerialBT.write(to_char(current_status));
    }
    else if (data == FORCE_STOP)
    {
      Serial.write("FORCE STOP BLE CMD ");
      forceStop();
    }
    else if (data == FIRST_FLOOR_PRESS)
    {
      elevatorLog("FIRST_FLOOR_PRESS BLE CMD ");
      moveToFloor(FIRST_FLOOR);
    }
    else if (data == SECOND_FLOOR_PRESS)
    {
      elevatorLog("SECOND_FLOOR_PRESS BLE CMD ");
      moveToFloor(SECOND_FLOOR);
    }
    else if (data == THIRD_FLOOR_PRESS)
    {
      elevatorLog("THIRD_FLOOR_PRESS BLE CMD ");
      moveToFloor(THIRD_FLOOR);
    }
    else if (data == DOOR_STATUS)
    {
      SerialBT.write(to_char(is_door_close));
    }
    else if (data == BOTTOM_STOP_STATUS)
    {
      SerialBT.write(digitalToString(bottomStopTrigger.getState()));
    }
    else
    {
      Serial.write(data);
    }
  }
  // send black box data to the mqtt server every 5 seconds
  if (millis() - lastPingTime > 5000)
  {
    sendBlackBox();
  }
}