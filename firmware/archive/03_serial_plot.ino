/*
 * 03_serial_plot
 * v3 - 串口绘图：逗号分隔输出，适配 Arduino Serial Plotter
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
    float accelX = ax / 16384.0;
    float accelY = ay / 16384.0;
    float accelZ = az / 16384.0;
    
    // 关键修改：在同一行用逗号分隔打印三个轴的数据
    Serial.print(accelX, 3);
    Serial.print(",");
    Serial.print(accelY, 3);
    Serial.print(",");
    Serial.println(accelZ, 3);  // 最后用 println 换行
    
    delay(50); // 适当降低发送频率，图表更清晰
}