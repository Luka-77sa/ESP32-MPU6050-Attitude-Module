/*
 * ESP32 + MPU6050 互补滤波姿态解算 + BLE 传输
 * 输出格式：Time(s),Roll(deg),Pitch(deg)
 * 优化：降低alpha，增大Ki，动态dt，加速度计低通滤波
 * 适用于静态和动态场景，减少跳变和漂移
 * 
 * 修改说明：
 * - 校准前增加 500ms 延时，提高稳定性
 * - 校准后检查偏置值，若过大则自动重新校准（最多重试2次）
 * - 强化姿态变量重置，确保从零开始
 */

#include <Wire.h>
#include <MPU6050.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

MPU6050 mpu;

// ---------- BLE 配置 ----------
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) { deviceConnected = true; }
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    BLEDevice::getAdvertising()->start();
  }
};

// ---------- 滤波参数（优化） ----------
float alpha = 0.90;               // 加速度计权重增大，快速修正
float Ki = 0.05;                  // 积分增益增强，加速收敛
float STATIC_THRESHOLD = 1.0;     // 放宽静止检测，避免误判

// ---------- 姿态变量 ----------
float roll = 0.0, pitch = 0.0;
float gyroRoll = 0.0, gyroPitch = 0.0;
float integralFB_roll = 0.0, integralFB_pitch = 0.0;

// ---------- 加速度计低通滤波 ----------
float accelRoll_filt = 0.0, accelPitch_filt = 0.0;
float accelFilterAlpha = 0.5;     // 系数 0.5，平滑适中

// ---------- 陀螺仪零偏 ----------
float gyroBiasX = 0.0, gyroBiasY = 0.0, gyroBiasZ = 0.0;
const int CALIBRATION_SAMPLES = 500;

// ---------- 时间 ----------
unsigned long lastTime = 0;

// ---------- 辅助函数 ----------
float radToDeg(float rad) { return rad * 180.0 / PI; }
float normalizeAngle(float angle) {
  while (angle > 180.0) angle -= 360.0;
  while (angle < -180.0) angle += 360.0;
  return angle;
}

// ---------- 陀螺仪校准（增强版） ----------
bool calibrateGyroscope(int retryCount = 0) {
  // 增加稳定延时，确保 MPU6050 内部状态稳定
  delay(500);
  
  Serial.println("Calibrating gyroscope... Keep sensor still!");
  int16_t gx, gy, gz;
  long sx = 0, sy = 0, sz = 0;
  
  // 采集数据
  for (int i = 0; i < CALIBRATION_SAMPLES; i++) {
    mpu.getRotation(&gx, &gy, &gz);
    sx += gx; sy += gy; sz += gz;
    delay(5);
  }
  
  // 计算偏置
  gyroBiasX = (sx / CALIBRATION_SAMPLES) / 131.0;
  gyroBiasY = (sy / CALIBRATION_SAMPLES) / 131.0;
  gyroBiasZ = (sz / CALIBRATION_SAMPLES) / 131.0;
  
  Serial.print("Bias X: "); Serial.print(gyroBiasX, 4); Serial.println(" °/s");
  Serial.print("Bias Y: "); Serial.print(gyroBiasY, 4); Serial.println(" °/s");
  Serial.print("Bias Z: "); Serial.print(gyroBiasZ, 4); Serial.println(" °/s");
  
  // 检查偏置是否过大（通常 > 0.8 可能表示校准期间传感器移动）
  if (abs(gyroBiasX) > 0.8 || abs(gyroBiasY) > 0.8 || abs(gyroBiasZ) > 0.8) {
    Serial.println("Warning: Bias too large. Recalibrating...");
    if (retryCount < 2) {
      return calibrateGyroscope(retryCount + 1);
    } else {
      Serial.println("Recalibration failed. Using current bias (may cause drift).");
      return false;
    }
  }
  
  Serial.println("Calibration done.");
  return true;
}

// ---------- 初始化 ----------
void setup() {
  Serial.begin(115200);
  Wire.begin();

  mpu.initialize();
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed!");
    while (1);
  }
  
  // 执行校准（可重试）
  calibrateGyroscope();

  // 重置所有姿态变量（确保从零开始）
  roll = 0.0; pitch = 0.0;
  gyroRoll = 0.0; gyroPitch = 0.0;
  integralFB_roll = 0.0; integralFB_pitch = 0.0;
  accelRoll_filt = 0.0; accelPitch_filt = 0.0;
  lastTime = micros();

  // ---------- BLE ----------
  BLEDevice::init("ESP32_MPU6050_BLE");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();
  BLEDevice::getAdvertising()->addServiceUUID(SERVICE_UUID);
  BLEDevice::getAdvertising()->start();

  Serial.println("BLE ready. Output: Time(s),Roll(deg),Pitch(deg)");
}

// ---------- 主循环 ----------
void loop() {
  unsigned long now = micros();
  float dt = (now - lastTime) / 1e6;
  lastTime = now;
  if (dt > 0.1) dt = 0.02;  // 防止启动时异常值

  int16_t ax, ay, az, gx, gy, gz;
  mpu.getAcceleration(&ax, &ay, &az);
  mpu.getRotation(&gx, &gy, &gz);

  float accelX = ax / 16384.0;
  float accelY = ay / 16384.0;
  float accelZ = az / 16384.0;

  float gyroX = (gx / 131.0) - gyroBiasX;
  float gyroY = (gy / 131.0) - gyroBiasY;
  float gyroZ = (gz / 131.0) - gyroBiasZ;

  // 加速度计原始角度（弧度）
  float accelRoll = atan2(accelY, accelZ);
  float accelPitch = atan2(-accelX, sqrt(accelY*accelY + accelZ*accelZ));

  // 低通滤波加速度计角度
  if (roll == 0.0 && pitch == 0.0) {
    accelRoll_filt = accelRoll;
    accelPitch_filt = accelPitch;
  } else {
    accelRoll_filt = accelFilterAlpha * accelRoll + (1 - accelFilterAlpha) * accelRoll_filt;
    accelPitch_filt = accelFilterAlpha * accelPitch + (1 - accelFilterAlpha) * accelPitch_filt;
  }

  // 静止检测（阈值宽松）
  if (abs(gyroX) < STATIC_THRESHOLD && 
      abs(gyroY) < STATIC_THRESHOLD && 
      abs(gyroZ) < STATIC_THRESHOLD) {
    // 静止：直接采用滤波后的加速度计角度，并重置积分器
    roll = accelRoll_filt;
    pitch = accelPitch_filt;
    gyroRoll = accelRoll_filt;
    gyroPitch = accelPitch_filt;
    integralFB_roll = 0.0;
    integralFB_pitch = 0.0;
  } else {
    // 运动：互补滤波（加大加速度计权重）
    float errorRoll = accelRoll_filt - roll;
    float errorPitch = accelPitch_filt - pitch;
    integralFB_roll += errorRoll * dt;
    integralFB_pitch += errorPitch * dt;
    integralFB_roll = constrain(integralFB_roll, -10.0, 10.0);
    integralFB_pitch = constrain(integralFB_pitch, -10.0, 10.0);
    gyroX += Ki * integralFB_roll;
    gyroY += Ki * integralFB_pitch;
    gyroRoll += gyroX * dt;
    gyroPitch += gyroY * dt;
    roll = alpha * gyroRoll + (1.0 - alpha) * accelRoll_filt;
    pitch = alpha * gyroPitch + (1.0 - alpha) * accelPitch_filt;
  }

  float rollDeg = normalizeAngle(radToDeg(roll));
  float pitchDeg = normalizeAngle(radToDeg(pitch));

  if (!isnan(rollDeg) && !isnan(pitchDeg)) {
    float timeSec = millis() / 1000.0;
    char buffer[50];
    snprintf(buffer, sizeof(buffer), "%.3f,%.2f,%.2f", timeSec, rollDeg, pitchDeg);
    Serial.println(buffer);
    if (deviceConnected) {
      pCharacteristic->setValue(buffer);
      pCharacteristic->notify();
    }
  }

  delay(20);  // 约 50Hz
}