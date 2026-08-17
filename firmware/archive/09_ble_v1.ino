/*
 * 09_ble_v1
 * v9 - BLE v1：无线姿态数据传输
 * 历史版本归档，详见 ../README.md 与 CHANGELOG.md
 */

#include <Wire.h>

// MPU6050 寄存器地址
#define MPU6050_ADDR         0x68
#define PWR_MGMT_1           0x6B
#define ACCEL_XOUT_H         0x3B
#define GYRO_XOUT_H          0x43

// 量程设置（此处使用 ±250°/s 陀螺仪，±2g 加速度计）
#define GYRO_SCALE           131.0   // 250°/s 时每 LSB 对应 131 LSB/°/s
#define ACCEL_SCALE          16384.0 // ±2g 时每 LSB 对应 16384 LSB/g

// 滤波参数
float TAU = 0.5;            // 时间常数（单位：秒），越大平滑度越高，响应越慢
float dt;                   // 采样间隔（秒）
unsigned long last_time;    // 上次采样时间（微秒）
const long sample_interval = 5000; // 目标采样间隔 5ms = 200Hz

// 角度变量
float accel_roll, accel_pitch;   // 由加速度计计算的角度（原始）
float gyro_roll, gyro_pitch;     // 陀螺仪积分角度
float filtered_roll, filtered_pitch; // 互补滤波输出

// 零点偏移（静态校准）
float roll_offset = 0.0, pitch_offset = 0.0;
bool calibrated = false;

// 原始数据
int16_t accel_x, accel_y, accel_z;
int16_t gyro_x, gyro_y, gyro_z;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(400000); // 快速 I2C

  // 初始化 MPU6050
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(PWR_MGMT_1);
  Wire.write(0x00);      // 唤醒，使用内部时钟
  Wire.endTransmission();
  delay(100);

  // 校准零点（静止水平放置 3 秒）
  calibrateSensor();
}

void loop() {
  // 固定采样周期
  if (micros() - last_time < sample_interval) return;
  dt = (micros() - last_time) / 1e6;  // 转换为秒
  last_time = micros();

  // 读取传感器
  readMPU6050();

  // 计算加速度计角度（单位：度）
  accel_roll = atan2(accel_y, accel_z) * 180.0 / PI;
  accel_pitch = atan2(-accel_x, sqrt(accel_y*accel_y + accel_z*accel_z)) * 180.0 / PI;

  // 减去零点偏移（校准后）
  if (calibrated) {
    accel_roll -= roll_offset;
    accel_pitch -= pitch_offset;
  }

  // 陀螺仪角速度（度/秒）
  float gyro_rate_x = gyro_x / GYRO_SCALE;   // Roll 角速度
  float gyro_rate_y = gyro_y / GYRO_SCALE;   // Pitch 角速度

  // ----- 互补滤波 -----
  // 首次运行时，将滤波角度初始化为加速度计角度
  if (!calibrated) {
    filtered_roll = accel_roll;
    filtered_pitch = accel_pitch;
  } else {
    // 陀螺仪积分
    gyro_roll += gyro_rate_x * dt;
    gyro_pitch += gyro_rate_y * dt;

    // 互补融合：角度 = 陀螺仪积分 + 加速度计修正
    float alpha = TAU / (TAU + dt);
    filtered_roll = alpha * (filtered_roll + gyro_rate_x * dt) + (1 - alpha) * accel_roll;
    filtered_pitch = alpha * (filtered_pitch + gyro_rate_y * dt) + (1 - alpha) * accel_pitch;
  }

  // 输出数据（CSV 格式）
  Serial.print(millis() / 1000.0, 3);  // 时间（秒）
  Serial.print(",");
  Serial.print(accel_roll, 2);
  Serial.print(",");
  Serial.print(accel_pitch, 2);
  Serial.print(",");
  Serial.print(filtered_roll, 2);
  Serial.print(",");
  Serial.println(filtered_pitch, 2);
}

// ----- 读取 MPU6050 原始数据 -----
void readMPU6050() {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(ACCEL_XOUT_H);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU6050_ADDR, 14, true); // 读取 14 字节（加速度+陀螺仪）

  accel_x = Wire.read() << 8 | Wire.read();
  accel_y = Wire.read() << 8 | Wire.read();
  accel_z = Wire.read() << 8 | Wire.read();
  Wire.read(); Wire.read(); // 温度跳过
  gyro_x = Wire.read() << 8 | Wire.read();
  gyro_y = Wire.read() << 8 | Wire.read();
  gyro_z = Wire.read() << 8 | Wire.read();
}

// ----- 零点校准（上电后水平静止）-----
void calibrateSensor() {
  Serial.println("Calibrating... Keep sensor still and level.");
  float sum_roll = 0, sum_pitch = 0;
  const int samples = 300; // 约 3 秒（200Hz * 3）
  
  for (int i = 0; i < samples; i++) {
    readMPU6050();
    float roll = atan2(accel_y, accel_z) * 180.0 / PI;
    float pitch = atan2(-accel_x, sqrt(accel_y*accel_y + accel_z*accel_z)) * 180.0 / PI;
    sum_roll += roll;
    sum_pitch += pitch;
    delay(5);  // 约 200Hz
  }
  roll_offset = sum_roll / samples;
  pitch_offset = sum_pitch / samples;
  calibrated = true;
  Serial.println("Calibration done.");
  Serial.println("Time(s),Raw_Roll,Raw_Pitch,Smooth_Roll,Smooth_Pitch");
  last_time = micros(); // 初始化时间基准
}