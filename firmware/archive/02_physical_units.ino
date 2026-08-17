/*
 * 02_physical_units
 * v2 - 物理单位：原始值转换为 g / deg/s
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
    int16_t gx, gy, gz;
    
    mpu.getAcceleration(&ax, &ay, &az);
    mpu.getRotation(&gx, &gy, &gz);
    
    // 将原始值转换为物理单位
    float accelX = ax / 16384.0;   // 单位: g
    float accelY = ay / 16384.0;
    float accelZ = az / 16384.0;
    
    float gyroX = gx / 131.0;      // 单位: °/s
    float gyroY = gy / 131.0;
    float gyroZ = gz / 131.0;
    
    // 打印转换后的数据
    Serial.print("Accel (g) X: "); Serial.print(accelX, 3);
    Serial.print(" | Y: "); Serial.print(accelY, 3);
    Serial.print(" | Z: "); Serial.print(accelZ, 3);
    
    Serial.print("  |  Gyro (°/s) X: "); Serial.print(gyroX, 2);
    Serial.print(" | Y: "); Serial.print(gyroY, 2);
    Serial.print(" | Z: "); Serial.println(gyroZ, 2);
    
    delay(500);
}
