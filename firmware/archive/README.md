# 固件历史版本归档 (Firmware Archive)

本目录保存了最终版之前的所有开发里程碑，按功能演进编号。每个文件都是完整可独立编译的 Arduino 工程（`.ino`）。

| 文件 | 版本 | 里程碑 |
| ---- | ---- | ------ |
| `01_basic_read.ino` | v1 | I2C 连接测试，读取加速度原始值 |
| `02_physical_units.ino` | v2 | 原始值 → 物理单位（g / °/s） |
| `03_serial_plot.ino` | v3 | 串口绘图格式输出（逗号分隔） |
| `04_plot_with_picture.ino` | v4 | 加速度计角度可视化 |
| `05_complementary_filter.ino` | v5 | 互补滤波融合，Roll / Pitch 解算 |
| `06_breadboard_filter.ino` | v6 | 面包板实测，滤波参数调优 |
| `07_bias_calibration.ino` | v7 | 引入陀螺仪零偏校准 |
| `08_static_drift_removal.ino` | v8 | 静止检测消除静态漂移（关键算法里程碑） |
| `09_ble_v1.ino` | v9 | BLE 无线传输 v1 |
| `10_ble_v2.ino` | v10 | BLE v2：低通滤波 + 动态 dt |

**最终版**（v11）位于 [`../ESP32_MPU6050_attitude/`](../ESP32_MPU6050_attitude/)，包含上述全部功能并增加了校准重试、姿态重置等生产级优化。

## 依赖库

所有版本依赖：

- `Wire`（Arduino 内置）
- `MPU6050`（Library Manager 安装）
- v9+ 额外依赖：`BLEDevice` / `BLEUtils` / `BLEServer` / `BLE2902`（ESP32 板级包内置）

## 版本演进图

```
v1 基本读取 ─► v2 物理单位 ─► v3 串口绘图 ─► v4 角度可视化
                                                       │
                                                       ▼
v5 互补滤波 ─► v6 面包板调优 ─► v7 零偏校准 ─► v8 消除静态漂移 ★核心算法
                                                       │
                                                       ▼
v9 BLE v1 ─► v10 BLE v2（低通+动态dt）─► v11 最终版（校准重试+重置）
```
