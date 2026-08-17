/*
 * 07_bias_calibration
 * v7 - 零偏校准：500 样本陀螺仪偏置估计
 * 历史版本归档，详见 ../README.md 与 CHANGELOG.md
 */

#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

// ---------- 滤波参数 ----------
float alpha = 0.96;      // 比例权重
float Ki = 0.1;         // 积分增益（消除静态漂移的关键）
float dt = 0.05;         // 采样周期

// ---------- 姿态变量 ----------
float roll = 0.0, pitch = 0.0;
float gyroRoll = 0.0, gyroPitch = 0.0;

// ---------- 积分补偿变量 ----------
float integralFB_roll = 0.0;
float integralFB_pitch = 0.0;

// ---------- 陀螺仪零偏校准 ----------
float gyroBiasX = 0.0, gyroBiasY = 0.0, gyroBiasZ = 0.0;
const int CALIBRATION_SAMPLES = 500;

float radToDeg(float rad) {
    return rad * 180.0 / 3.14159265;
}

float normalizeAngle(float angle) {
    while (angle > 180.0) angle -= 360.0;
    while (angle < -180.0) angle += 360.0;
    return angle;
}

void calibrateGyroscope() {
    Serial.println("Calibrating... Keep sensor still!");
    delay(1000);
    int16_t gx, gy, gz;
    long sx = 0, sy = 0, sz = 0;
    for (int i = 0; i < CALIBRATION_SAMPLES; i++) {
        mpu.getRotation(&gx, &gy, &gz);
        sx += gx; sy += gy; sz += gz;
        delay(5);
    }
    gyroBiasX = (sx / CALIBRATION_SAMPLES) / 131.0;
    gyroBiasY = (sy / CALIBRATION_SAMPLES) / 131.0;
    gyroBiasZ = (sz / CALIBRATION_SAMPLES) / 131.0;
    Serial.println("Calibration done!");
}

void setup() {
    Serial.begin(115200);
    Wire.begin();
    mpu.initialize();
    if (!mpu.testConnection()) {
        Serial.println("MPU6050 connection failed!");
        while (1);
    }
    calibrateGyroscope();
    delay(500);
}

void loop() {
    int16_t ax, ay, az, gx, gy, gz;
    mpu.getAcceleration(&ax, &ay, &az);
    mpu.getRotation(&gx, &gy, &gz);

    float accelX = ax / 16384.0;
    float accelY = ay / 16384.0;
    float accelZ = az / 16384.0;
    float gyroX = (gx / 131.0) - gyroBiasX;
    float gyroY = (gy / 131.0) - gyroBiasY;
    float gyroZ = (gz / 131.0) - gyroBiasZ;

    // ---------- 1. 加速度计计算静态角度 ----------
    float accelRoll = atan2(accelY, accelZ);
    float accelPitch = atan2(-accelX, sqrt(accelY*accelY + accelZ*accelZ));

    // ---------- 2. 计算误差（用于积分补偿） ----------
    float errorRoll = accelRoll - roll;
    float errorPitch = accelPitch - pitch;

    // ---------- 3. 积分累加误差 ----------
    integralFB_roll += errorRoll * dt;
    integralFB_pitch += errorPitch * dt;

    // ---------- 4. 限幅防溢出 ----------
    integralFB_roll = constrain(integralFB_roll, -10.0, 10.0);
    integralFB_pitch = constrain(integralFB_pitch, -10.0, 10.0);

    // ---------- 5. 修正陀螺仪输入 ----------
    gyroX += Ki * integralFB_roll;
    gyroY += Ki * integralFB_pitch;

    // ---------- 6. 陀螺仪积分 ----------
    gyroRoll += gyroX * dt;
    gyroPitch += gyroY * dt;

    // ---------- 7. 互补滤波融合 ----------
    roll = alpha * gyroRoll + (1.0 - alpha) * accelRoll;
    pitch = alpha * gyroPitch + (1.0 - alpha) * accelPitch;

    // ---------- 8. 归一化输出 ----------
    float rollDeg = normalizeAngle(radToDeg(roll));
    float pitchDeg = normalizeAngle(radToDeg(pitch));

    if (!isnan(rollDeg) && !isnan(pitchDeg)) {
        Serial.print("Roll:");
        Serial.print(rollDeg, 2);
        Serial.print(",Pitch:");
        Serial.println(pitchDeg, 2);
    }
    delay(50);
}