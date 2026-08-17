# Changelog

本文件记录 ESP32 + MPU6050 姿态采集系统固件的全部开发历程，按时间倒序排列。

当前仓库仅保留最终版（v11）主工程；早期版本（v1~v10）源码已归档，此记录用于追溯演进过程。

---

## v11 — 最终版 (ESP32_MPU6050_attitude.ino)

**状态**: 当前主工程 · 生产可用

**改进**
- 校准前增加 **500 ms 稳定延时**，提高偏置估计可靠性
- 校准后检查偏置，|bias| > 0.8 °/s 时**自动重试**（最多 2 次）
- 校准后**重置全部姿态变量**，确保从零开始
- 加速度计角度增加**低通滤波**（`accelFilterAlpha = 0.5`）
- **动态 dt**：`micros()` 计时 + 上限截断（0.1 s → 0.02 s）
- 静止检测阈值放宽至 1.0 °/s，避免运动误判
- 输出前 `isnan` 保护，BLE Notify ~50 Hz

---

## v10 — BLE v2 (10_ble_v2.ino)

- `alpha` 降至 0.90（加大加速度计权重，快速修正）
- `Ki` 增至 0.05，加快收敛
- 静止阈值放宽至 1.0 °/s
- 引入动态 `dt` 与加速度计低通滤波
- BLE 输出 `Time(s),Roll(deg),Pitch(deg)`

## v9 — BLE v1 (09_ble_v1.ino)

- 首个无线版本：BLE GATT Server（`ESP32_MPU6050_BLE`），Notify + Read
- 断连后自动重新广播
- 串口输出原始与平滑数据（Time, Raw_Roll, Raw_Pitch, Smooth_Roll, Smooth_Pitch）

## v8 — 静止检测消除静态漂移 (08_static_drift_removal.ino) ★关键算法里程碑

- **静止检测**：三轴角速度均 < 0.15 °/s 时，角度强制跟随加速度计读数
- 静止时重置积分状态，彻底消除长期漂移
- 保留 500 样本零偏校准（含 1000 ms 稳定延时）

## v7 — 零偏校准 (07_bias_calibration.ino)

- 上电 500 样本陀螺仪偏置均值估计
- `Ki` 提高至 0.1，主动抵消残余静态漂移

## v6 — 面包板滤波调优 (06_breadboard_filter.ino)

- 首个互补滤波实现：`alpha = 0.96`，`Ki`，固定 `dt = 0.05 s`（面包板实测调参）

## v5 — 互补滤波 (05_complementary_filter.ino)

- `atan2` 计算加速度计角度（Roll / Pitch，弧度）
- 互补融合：`angle = alpha * gyro + (1-alpha) * accel`
- 角度归一化至 (-180, 180]

## v4 — 角度可视化 (04_plot_with_picture.ino)

- 加速度计角度输出，适配串口绘图

## v3 — 串口绘图 (03_serial_plot.ino)

- 逗号分隔三轴数据输出，适配 Arduino Serial Plotter

## v2 — 物理单位 (02_physical_units.ino)

- 加速度 16384 LSB/g、陀螺仪 131 LSB/(°/s) 换算
- 输出 g 与 °/s

## v1 — 基础读取 (01_basic_read.ino)

- I2C 连接测试（`Wire` + `MPU6050`）
- 串口输出加速度原始值（115200）
