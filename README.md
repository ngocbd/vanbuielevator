# Thiết kế Mạch Điều Khiển thăng máy tự chế cho nhà ông Bùi Đình Vân

## Sơ đồ Thiết kế

           ___
          | E |
          |___|
           |||
           |||
     ______|||
    |      ||| 
    |______|||
           |||
           |||
     ______|||
    |      |||
    |______|||
           |||
           |||
     ______|||
    |      |||
    |______|||

- Thang máy gồm 1 động cơ trên nóc tầng 3
- Thang máy được cân bằng bằng đối trọng nằng trong ray
- Thang máy được cố định vào ray, chỉ có thể di chuyển lên và xuống
- Hệ thống phanh khẩn cấp bằng gia tốc hoặc tốc độ ( cần viết chi tiết )
- Giai đoạn một sẽ dùng động cơ là tời kéo
- Hệ thống điều khiển bằng ESP32

## Sơ đồ mạch điều khiển ESP32

![ Sơ đồ mạch điều khiển ESP32 DOIT Devkit v1 ](https://mischianti.org/wp-content/uploads/2020/11/ESP32-DOIT-DEV-KIT-v1-pinout-mischianti.png)

## Sơ đồ mạch demo

![ Sơ đồ mạch demo ](./wiring-diagram.png)

## Các thành phần

### 1. Nút gọi thang

- 6 nút thang máy (3 trong cabin, 3 ở các tầng)
    - 6 nút này cùng chức năng (logic)
    - Được setup INPUT_PULLUP nên cần nối 1 đầu với GND. Đầu còn lại với esp32 pin
    - Các nút này sẽ được điều khiển bằng thư viện ezButton (xử lý debounce)
    - pinout có 4 pin:  1,2 cho led, 3 4 cho công tắc
    - Led các nút được điều khiển bằng relay

      | pin no |    connect to    |
                                                                                                                                          |:-------|:----------------:|
      | 1      |  LED VCC (12v)   | 
      | 2      |  LED GND (12V)   |   
      | 3      | ESP 32 (PULL_UP) |
      | 4      |    ESP 32 GND    |

### 2. Công tắc hành trình mỗi tầng

![limit-switch-pinout.jpg](./images/limit-switch-pinout.jpg)

- Để xác định thang máy đã đến tầng nào có 3 limit switch(công tắc hành trình mỗi tầng)
- 3 nút này cũng dùng INPUT_PULLUP với chế độ thường mở (NO)

| C pin | No Pin                         | NC pin        | ESP32 Input state                      |  
|-------|--------------------------------|---------------|----------------------------------------|
| GND   | ESP32 Input Pin (with pull-up) | not connected | LOW when touched,  HIGH when untouched |  

### 3. Công tắc an toàn ở dưới đáy thang

- Dùng để dừng thang máy khi có vật cản ở dưới đáy thang
- Dùng INPUT_PULLUP với chế độ thường đóng (NC)
- khi pnw signal = HIGH => Có vật cản => dừng thang máy

### 4. Công tắc cửa thang máy

- Dùng để xác định cửa thang máy đã đóng hay chưa
- Dùng INPUT_PULLUP với chế độ thường mở (NO)
- Khi cửa đóng => signal = LOW

### 5. Động cơ kéo thang

- Gồm 2 động cơ tời để kéo thang máy lên / xuống
- Động cơ này sẽ được điều khiển bằng 2 relay

## PIN Definition

### 1. Nút gọi thang

```cpp
#define ONEST_FLOOR 18
#define TWOND_FLOOR 19
#define THREE_FLOOR 21

#define ONEST_FLOOR_LED_PIN 13
#define TWOND_FLOOR_LED_PIN 12
#define THREE_FLOOR_LED_PIN 14
```

### 2. Công tắc hành trình mỗi tầng

```cpp
#define TRIGGER_FIRST_FLOOR_DOWN 25
#define TRIGGER_SECOND_FLOOR_DOWN 26
#define TRIGGER_THIRD_FLOOR_DOWN 27
```

### 3. Công tắc an toàn ở dưới đáy thang

```cpp
#define BOTTOM_STOP_TRIGGER 32
```

### 4. Công tắc cửa thang máy

```cpp
#define DOOR_CLOSE_TRIGGER 33
```

## Điều khiển qua bluetooth

- Ngoài việc điều khiển thang máy qua nút nhấn, ta cũng có thể điều khiển thang máy qua bluetooth
- Connect với ESP32 qua bluetooth: `ONGNOI`
- List lệnh

| cmd |              purpose              |                          return  value |
|:----|:---------------------------------:|---------------------------------------:|
| w   |        thang máy tầng  nào        |                            1 or 2 or 3 |
| s   |       Trạng thái thang máy        | 1(đi lên), 2 (đi xuống), 0 (đứng  yên) |
| 1   |       Gọi thang lên tầng 1        |                                        |
| 2   |       Gọi thang lên tầng 2        |                                        |
| 2   |       Gọi thang lên tầng 3        |                                        |
| t   | PNW signal của trigger từng tầng  |                       xxx(x is H or L) |
| f   |            Force  stop            |                                        |
| d   |          is door close ?          |                        0 (Yes) 1  (No) |
| d   | PNW signal của công tắc đáy thang |                                 H or L |

```cpp
#define WHERE_ARE_YOU 'w'
#define WHAT_STATUS 's' // ENEGINE UP, DOWN, STOP status
#define FIRST_FLOOR_PRESS '1'
#define SECOND_FLOOR_PRESS '2'
#define THIRD_FLOOR_PRESS '3'
#define TRIGGER_STATUS 't' // TRIGGER STATUS
#define FORCE_STOP 'f'
#define DOOR_STATUS 'd'
#define BOTTOM_STOP_STATUS 'b'
```

## MQTT for monitoring

```cpp
const char *mqtt_server = "103.199.18.51";
//logs function
void elevatorLog(String message)
{
  Serial.println(message);
  mqttLogger.println(message);
  SerialBT.println(message);
}
//blackbox function
void sendBlackBox()
{
  // send the black box data to the mqtt server
  char blackBoxMessage[110]; // Allocate a buffer for the log message
  // build json string
  snprintf(blackBoxMessage, sizeof(blackBoxMessage), "{\"current_pos\": %d, \"current_status\": %d, \"target_floor\": %d, \"is_door_close\": %d, \"is_pausing\": %d}", current_pos, current_status, target_floor, is_door_close, is_pausing);
  mqttLogger.println(blackBoxMessage);
  lastPingTime = millis();
}
```

- Do thang máy hay chết lỗi ko rõ lý do nên cần monitor thang máy qua MQTT
- Logs lại các sự kiện của thang máy, gửi blackbox data mỗi 5s qua MQTT
- Theo dõi logs qua: https://vanbuielevator-monitor.pages.dev/

## Watchdog

```cpp
  // init wdt
  esp_task_wdt_init(WDT_TIMEOUT, true); // enable panic so ESP32 restarts
  esp_task_wdt_add(NULL);
```

- Xuất hiện lỗi treo mạch khi để lâu
- Implement watchdog để reset mạch khi treo (sau 30s nếu hàm `loop()`  ko chạy)

## Quá trình vận hành

1. Gọi thang máy đến tầng X ( thang máy có thể đang ở bất kỳ đâu)
2. Người lên => Vận chuyển đến tầng Y ( bước 1 và 2 có thể xảy ra nhiều lần)

## Bảo hiểm an toàn

- Gồm 2 công tắc kịch trần chạm đáy tác động cơ học lên động cơ đẻ dừng động cơ ko qua esp32
- Cầu chì đảm bảo ko cho động cơ lên và xuống chạy cùng lúc
- Công tắc an tòàn đáy thang
- Công tắc cửa thang
```cpp
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
```
## Quyền sudo

- Điều khiển động cơ lên xuống dừng qua sóng radio bằng điều khiển từ xa
- Không cần qua mạch =>  độ tin cậy cao nhất

## Các vấn đề cần giải quyết

- Lỗi treo mạch  (đã fix tạm bằng cách thêm wathdog)
- Thang tự lên tầng 3 sau 1 2h ngồi im (đang tìm hiểu)
- Led nút gọi thang (đã code logic cần đi dây thực tiễn)
- Quản trị vaf điều khiển qua Internet: web, telegram (đang làm)
- Phát audio khi thang máy đến tầng (chưa làm)