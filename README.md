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
