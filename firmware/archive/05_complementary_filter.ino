/*
 * 05_complementary_filter
 * v5 - 互补滤波：Roll/Pitch 姿态解算
 * 历史版本归档，详见 ../README.md 与 CHANGELOG.md
 */

/*
 ESP32 + MPU6050 互补滤波姿态解算
 修复：大角度偏转时卡死在 ±180° 边界的问题
 
 硬件连接:
 MPU6050 VCC -> ESP32 3.3V
 MPU6050 GND -> ESP32 GND
 MPU6050 SCL -> ESP32 GPIO 22
 MPU6050 SDA -> ESP32 GPIO 21
 

#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

// ============================================================
// 滤波参数
// ============================================================
float alpha = 0.97;           // 互补滤波权重 (0.95~0.98)
float dt = 0.05;              // 采样周期 (秒)

// ============================================================
// 姿态变量
// ============================================================
float roll = 0.0;
float pitch = 0.0;
float gyroRoll = 0.0;
float gyroPitch = 0.0;

// ============================================================
// 陀螺仪零偏校准变量
// ============================================================
float gyroBiasX = 0.0;
float gyroBiasY = 0.0;
float gyroBiasZ = 0.0;
const int CALIBRATION_SAMPLES = 200;
bool calibrated = false;

// ============================================================
// 辅助函数: 弧度 ↔ 角度
// ============================================================
float radToDeg(float rad) {
    return rad * 180.0 / 3.14159265;
}

// ============================================================
// 角度归一化: 将任意角度映射到 -180° ~ 180°
// 修复边界卡死的核心函数
// ============================================================
float normalizeAngle(float angle) {
    while (angle > 180.0) angle -= 360.0;
    while (angle < -180.0) angle += 360.0;
    return angle;
}

// ============================================================
// 陀螺仪零偏校准
// ============================================================
void calibrateGyroscope() {
    Serial.println("🔧 开始陀螺仪零偏校准...");
    Serial.println("📌 请保持传感器绝对静止！");
    delay(1000);
    
    int16_t gx, gy, gz;
    long sumX = 0, sumY = 0, sumZ = 0;
    
    for (int i = 0; i < CALIBRATION_SAMPLES; i++) {
        mpu.getRotation(&gx, &gy, &gz);
        sumX += gx;
        sumY += gy;
        sumZ += gz;
        delay(5);
    }
    
    gyroBiasX = (sumX / CALIBRATION_SAMPLES) / 131.0;
    gyroBiasY = (sumY / CALIBRATION_SAMPLES) / 131.0;
    gyroBiasZ = (sumZ / CALIBRATION_SAMPLES) / 131.0;
    
    calibrated = true;
    
    Serial.print("   X轴零偏: "); Serial.print(gyroBiasX, 3); Serial.println(" °/s");
    Serial.print("   Y轴零偏: "); Serial.print(gyroBiasY, 3); Serial.println(" °/s");
    Serial.print("   Z轴零偏: "); Serial.print(gyroBiasZ, 3); Serial.println(" °/s");
    Serial.println("✅ 校准完成！开始输出数据...");
    Serial.println("-");
}

// ============================================================
// 设置
// ============================================================
void setup() {
    Serial.begin(115200);
    Wire.begin();
    
    mpu.initialize();
    
    if (!mpu.testConnection()) {
        Serial.println("❌ MPU6050 连接失败！请检查接线。");
        while (1);
    }
    
    Serial.println("✅ MPU6050 已连接");
    Serial.println("📐 互补滤波姿态解算 (alpha = " + String(alpha) + ")");
    Serial.println("-");
    
    delay(500);
    calibrateGyroscope();
}

// ============================================================
// 主循环
// ============================================================
void loop() {
    int16_t ax, ay, az;
    int16_t gx, gy, gz;
    
    // 1. 读取原始数据
    mpu.getAcceleration(&ax, &ay, &az);
    mpu.getRotation(&gx, &gy, &gz);
    
    // 2. 转换为物理单位
    float accelX = ax / 16384.0;
    float accelY = ay / 16384.0;
    float accelZ = az / 16384.0;
    
    float gyroX = (gx / 131.0) - gyroBiasX;
    float gyroY = (gy / 131.0) - gyroBiasY;
    float gyroZ = (gz / 131.0) - gyroBiasZ;
    
    // 3. 加速度计计算静态角度（-180° ~ 180°）
    float accelRoll = atan2(accelY, accelZ);
    float accelPitch = atan2(-accelX, sqrt(accelY * accelY + accelZ * accelZ));
    
    // 4. 陀螺仪积分（角度累加，无边界限制）
    gyroRoll += gyroX * dt;
    gyroPitch += gyroY * dt;
    
    // 5. 互补滤波融合
    roll = alpha * gyroRoll + (1.0 - alpha) * accelRoll;
    pitch = alpha * gyroPitch + (1.0 - alpha) * accelPitch;
    
    // ============================================================
    // 6. 边界卡死防护
    //    当融合角度与加速度计角度偏差 > 30° 时，强制重置
    //    防止因角度跳变导致的卡死
    // ============================================================
    float diffRoll = fabs(roll - accelRoll);
    float diffPitch = fabs(pitch - accelPitch);
    
    if (diffRoll > 0.5) {   // 0.5 弧度 ≈ 28°
        roll = accelRoll;
        gyroRoll = accelRoll;
    }
    if (diffPitch > 0.5) {
        pitch = accelPitch;
        gyroPitch = accelPitch;
    }
    
    // 7. 转换为角度并归一化到 -180° ~ 180°
    float rollDeg = normalizeAngle(radToDeg(roll));
    float pitchDeg = normalizeAngle(radToDeg(pitch));
    
    // 8. 输出（带防 nan 保护）
    if (!isnan(rollDeg) && !isnan(pitchDeg)) {
        Serial.print("Roll:");
        Serial.print(rollDeg, 2);
        Serial.print(",Pitch:");
        Serial.println(pitchDeg, 2);
    }
    
    delay(50);
}