﻿# Thiết kế Mạch Điều Khiển thăng máy tự chế cho nhà ông Bùi Đình Vân

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
- Thang máy bình thường sẽ nằm ở tầng 2 để không bị vướng lối đi lại ở tầng 1 .
- Thang máy được cân bằng bằng đối trọng nằng trong ray
- Thang máy được cố định vào ray, chỉ có thể di chuyển lên và xuống
- Hệ thống phanh khẩn cấp bằng gia tốc hoặc tốc độ ( cần viết chi tiết )
- Giai đoạn một sẽ dùng động cơ là tời kéo
- Hệ thống điều khiển bằng ESP32

## Sơ đồ mạch điều khiển ESP32

- Cài đặt cơ bản tốc độ cổng serial ( monitor)

```c++
Serial.begin(115200);
```
monitor_speed = 115200
- Cài đặt điều khiển bằng Bluetooth tên thiết bị là ONGNOI

```c++

SerialBT.begin("ONGNOI"); //Bluetooth device name
```

- Chân điều khiển động cơ kéo lên là chân 23 ( được setup OUTPUT)
- Chân điều khiển động cơ nhả xuống là chân 22 ( được setup OUTPUT)

```C++
#define ENGINE_UP_PIN 23
#define ENGINE_DOWN_PIN 22

//Setup pins mode for the engine
  pinMode(ENGINE_UP_PIN, OUTPUT);
  pinMode(ENGINE_DOWN_PIN, OUTPUT);

```
- Chân điều nhận tín hiệu từ cảm biến khi đến các tầng 1,2,3 lần lượt là 25,26,27 ( được setup INPUT)
  
```C++
#define TRIGGER_FIRST_FLOOR_DOWN 25
#define TRIGGER_SECOND_FLOOR_DOWN 26
#define TRIGGER_THIRD_FLOOR_DOWN 27
 // setup pins mode for trigger
  pinMode(TRIGGER_FIRST_FLOOR_DOWN, INPUT);
  pinMode(TRIGGER_SECOND_FLOOR_DOWN, INPUT);
  pinMode(TRIGGER_THIRD_FLOOR_DOWN, INPUT);
```

- Tập lệnh điều khiển từ Bluetooth


```c++
#define WHERE_ARE_YOU 'w'
#define WHAT_STATUS 's'
#define FIRST_FLOOR_PRESS '1'
#define SECOND_FLOOR_PRESS '2'
#define THIRD_FLOOR_PRESS '3'
```

- Lệnh w dùng để hỏi xem thang máy đang ở đâu ( kết quả trả về cho thiết bị hỏi là một trong các giá trị 1,2,3 tương ứng với 3 tầng như dưới)
```c++
#define FIRST_FLOOR 1
#define SECOND_FLOOR 2
#define THIRD_FLOOR 3

if(data == WHERE_ARE_YOU){
      Serial.write("WHERE_ARE_YOU CMD ");
      SerialBT.write(to_char(current_pos));
    }

```
- Lệnh s dùng để hỏi trạng thái của thang máy ( kết quả sẽ là đang lên, đang xuống , đang đứng yên)
```c++
#define UP 1
#define DOWN 2
#define STOP 0

if(data == WHAT_STATUS){
      Serial.write("WHAT_STATUS CMD ");
      SerialBT.write(to_char(current_status));
    }
```

## Quá trình vận hành
### Quá trình vận hành thang máy cần 3 giai đoạn 
1.Gọi thang máy đến tầng X ( thang máy có thể đang ở bất kỳ đâu)
2.Người lên => Vận chuyển đến tầng Y
3.Thang máy rảnh và tự trở về tầng 2 ngủ
