/*
 * 01_basic_read
 * v1 - 基础读取：I2C 连接测试 + 加速度原始值读取
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
    while (1);
  }
}

void loop() {
  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);

  Serial.print("Accel X: "); Serial.print(ax);
  Serial.print(" | Y: "); Serial.print(ay);
  Serial.print(" | Z: "); Serial.println(az);

  delay(500);
}