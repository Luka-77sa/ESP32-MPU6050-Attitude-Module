/*
 * 06_breadboard_filter
 * v6 - 面包板版：互补滤波参数调优
 * 历史版本归档，详见 ../README.md 与 CHANGELOG.md
 */

#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  mpu.initialize();

  if (mpu.testConnection()) {
    Serial.println("MPU6050 connected successfully!");
  } else {
    Serial.println("MPU6050 connection failed");
    while (1)
      ;
  }
}

void loop() {
  int16_t ax, ay, az;
  int16_t gx, gy, gz;

  // 读取原始数据
  mpu.getAcceleration(&ax, &ay, &az);
  mpu.getRotation(&gx, &gy, &gz);

  // 转换为物理单位
  float accelX = ax / 16384.0;  // ±2g 量程
  float accelY = ay / 16384.0;
  float accelZ = az / 16384.0;

  float gyroX = gx / 131.0;  // ±250°/s 量程
  float gyroY = gy / 131.0;
  float gyroZ = gz / 131.0;

  // 打印带标签的数据（串口绘图器可识别）
  Serial.print("AccelX:");
  Serial.print(accelX, 3);
  Serial.print(",AccelY:");
  Serial.print(accelY, 3);
  Serial.print(",AccelZ:");
  Serial.print(accelZ, 3);
  Serial.print(",GyroX:");
  Serial.print(gyroX, 2);
  Serial.print(",GyroY:");
  Serial.print(gyroY, 2);
  Serial.print(",GyroZ:");
  Serial.println(gyroZ, 2);  // 最后用 println 换行

  delay(50);  // 控制发送频率（20Hz）
}