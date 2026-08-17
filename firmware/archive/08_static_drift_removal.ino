/*
 * 08_static_drift_removal
 * v8 - 静止检测：彻底消除静态漂移（关键算法里程碑）
 * 历史版本归档，详见 ../README.md 与 CHANGELOG.md
 */

/*
 * ESP32 + MPU6050 互补滤波姿态解算
 * 特性：静止检测 + 零偏自动修正，彻底消除静态漂移
 * 输出格式：Roll:xx.xx,Pitch:xx.xx
 * 波特率：115200，采样率：20Hz
 */

#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

// ---------- 滤波参数filter parameters ----------
float alpha = 0.96;        // 互补滤波权重(Complementary filtering weights (0.95~0.98))
float Ki = 0.02;           // 积分增益（常规值）Integral gain (normal value)
float dt = 0.05;           // 采样周期 (秒)Sampling period (seconds)

// ---------- 静止检测阈值Static detection threshold ----------
float STATIC_THRESHOLD = 0.15;   // 角速度阈值 (°/s)，低于此值视为静止Angular velocity threshold (°/s); values below this threshold are considered to indicate rest.

// ---------- 姿态变量Attitude variables ----------
float roll = 0.0;
float pitch = 0.0;
float gyroRoll = 0.0;
float gyroPitch = 0.0;

// ---------- 积分补偿变量Integral compensation variable ----------
float integralFB_roll = 0.0;
float integralFB_pitch = 0.0;

// ---------- 陀螺仪零偏校准 Gyroscope zero bias calibration----------
float gyroBiasX = 0.0;
float gyroBiasY = 0.0;
float gyroBiasZ = 0.0;
const int CALIBRATION_SAMPLES = 500;

// ---------- 辅助函数 Auxiliary function----------
float radToDeg(float rad) {
    return rad * 180.0 / 3.14159265;
}

float normalizeAngle(float angle) {
    while (angle > 180.0) angle -= 360.0;
    while (angle < -180.0) angle += 360.0;
    return angle;
}

// ---------- 陀螺仪零偏校准Gyroscope zero bias calibration ----------
void calibrateGyroscope() {
    Serial.println("Calibrating gyroscope... Keep sensor still!");
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

    Serial.print("Bias X: "); Serial.print(gyroBiasX, 4); Serial.println(" °/s");
    Serial.print("Bias Y: "); Serial.print(gyroBiasY, 4); Serial.println(" °/s");
    Serial.print("Bias Z: "); Serial.print(gyroBiasZ, 4); Serial.println(" °/s");
    Serial.println("Calibration complete.");
    Serial.println("---");
}

// ---------- 初始化 Initialization----------
void setup() {
    Serial.begin(115200);
    Wire.begin();

    mpu.initialize();

    if (!mpu.testConnection()) {
        Serial.println("MPU6050 connection failed!");
        while (1);
    }

    Serial.println("MPU6050 connected.");
    calibrateGyroscope();

    // 重置所有姿态变量Reset all posture variables
    roll = 0.0;
    pitch = 0.0;
    gyroRoll = 0.0;
    gyroPitch = 0.0;
    integralFB_roll = 0.0;
    integralFB_pitch = 0.0;

    Serial.println("Ready. Output format: Roll:xx.xx,Pitch:xx.xx");
}

// ---------- 主循环 main loop----------
void loop() {
    int16_t ax, ay, az;
    int16_t gx, gy, gz;

    mpu.getAcceleration(&ax, &ay, &az);
    mpu.getRotation(&gx, &gy, &gz);

    float accelX = ax / 16384.0;
    float accelY = ay / 16384.0;
    float accelZ = az / 16384.0;

    float gyroX = (gx / 131.0) - gyroBiasX;
    float gyroY = (gy / 131.0) - gyroBiasY;
    float gyroZ = (gz / 131.0) - gyroBiasZ;

    // ---------- 加速度计计算静态角度 The accelerometer is used to calculate the static angle.----------
    float accelRoll = atan2(accelY, accelZ);
    float accelPitch = atan2(-accelX, sqrt(accelY * accelY + accelZ * accelZ));

    // ============================================================
    // 核心：静止检测 + 强制归零Core: Static detection + forced zeroing
    // 当三轴角速度都小于阈值时，认为传感器静止  When the angular velocities of all three axes are below the threshold, it is assumed that the sensor is at rest.
    // 此时直接把角度拉向加速度计角度，彻底消除漂移 At this point, the angle is adjusted to match that of the accelerometer, thereby eliminating any drift completely.
    // ============================================================
    if (abs(gyroX) < STATIC_THRESHOLD && 
        abs(gyroY) < STATIC_THRESHOLD && 
        abs(gyroZ) < STATIC_THRESHOLD) {
        // 静止状态：强制跟随加速度计  Stationary state: Forceful follow of the accelerometer
        roll = accelRoll;
        pitch = accelPitch;
        gyroRoll = accelRoll;
        gyroPitch = accelPitch;
        integralFB_roll = 0.0;
        integralFB_pitch = 0.0;
    } else {
        // ---------- 运动状态：正常互补滤波 Motion state: Normal complementary filtering----------
        // 计算误差 Calculation error
        float errorRoll = accelRoll - roll;
        float errorPitch = accelPitch - pitch;

        // 积分累加 Integral accumulation
        integralFB_roll += errorRoll * dt;
        integralFB_pitch += errorPitch * dt;

        // 限幅防溢出 Limiting to prevent overflow
        integralFB_roll = constrain(integralFB_roll, -10.0, 10.0);
        integralFB_pitch = constrain(integralFB_pitch, -10.0, 10.0);

        // 修正陀螺仪 Corrected gyroscope
        gyroX += Ki * integralFB_roll;
        gyroY += Ki * integralFB_pitch;

        // 陀螺仪积分 Gyroscope integration
        gyroRoll += gyroX * dt;
        gyroPitch += gyroY * dt;

        // 互补滤波融合 Complementary filtering fusion
        roll = alpha * gyroRoll + (1.0 - alpha) * accelRoll;
        pitch = alpha * gyroPitch + (1.0 - alpha) * accelPitch;
    }

    // ---------- 输出 Output----------
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
