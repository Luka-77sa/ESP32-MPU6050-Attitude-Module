# ESP32 + MPU6050 独立式 IMU 姿态采集系统

![Platform](https://img.shields.io/badge/Platform-ESP32-green) ![Sensor](https://img.shields.io/badge/Sensor-MPU6050%20(GY--521)-blue) ![License](https://img.shields.io/badge/License-MIT-yellow) ![Language](https://img.shields.io/badge/Language-Arduino%20C%2B%2B-orange)

面向四足机器人的轻量化独立式 IMU 姿态采集系统，基于 **NodeMCU-32S (ESP32)** 与 **GY-521 (MPU6050)**，实现互补滤波姿态解算（Roll / Pitch）、静态漂移消除、零偏自动校准与 BLE 无线传输，并配套自研 PCB、测量数据与分析脚本。

| 模块 | 说明 |
| ---- | ---- |
| **主控** | NodeMCU-32S（ESP32 双核，主频 80~240 MHz，4 MB Flash，Wi-Fi / BLE 4.2） |
| **传感器** | GY-521（MPU6050 六轴 IMU，I2C 地址 0x68） |
| **电源** | 3.7 V 锂电池 → SX1308 升压 5.0 V → HX9193-33GB LDO 稳压 3.3 V（含 ESD 防护） |
| **通信** | BLE Notify 无线传输 + USB 串口 115200 |
| **算法** | 互补滤波 + 静止检测 + 陀螺仪零偏自动校准 + 动态采样间隔 |

**核心亮点**

- 🎯 **互补滤波**：融合加速度计（低频精度高）与陀螺仪（高频响应好），输出 Roll / Pitch
- 🧹 **消除静态漂移**：静止检测（三轴角速度低于阈值）时强制跟随加速度计角度并重置积分器
- 🔄 **零偏自动校准**：上电采样 500 个样本求均值作为陀螺仪偏置，偏置异常自动重试（最多 2 次）
- ⏱ **动态 dt**：`micros()` 高精度计时，实时计算真实采样间隔
- 📡 **BLE 无线传输**：输出 `Time(s),Roll(deg),Pitch(deg)`，支持手机/PC 实时订阅

---

## 目录 (Contents)

- [快速开始 Quick Start](#快速开始-quick-start)
- [数据输出格式 Data Output Format](#数据输出格式-data-output-format)
- [项目结构 Project Structure](#项目结构-project-structure)
- [固件开发历程 Firmware History](#固件开发历程-firmware-history)
- [测量数据与分析 Measurement & Analysis](#测量数据与分析-measurement--analysis)
- [PCB 硬件设计 PCB Design](#pcb-硬件设计-pcb-design)
- [许可证 License](#许可证-license)

## 快速开始 Quick Start

### 环境依赖

- Arduino IDE ≥ 1.8.10 + ESP32 板级包（Boards Manager 安装 `esp32 by Espressif Systems`）
- 库：`MPU6050`（Library Manager）、`Wire`、`BLEDevice` 等（板级包内置）

### 烧录固件

1. 打开 [`firmware/ESP32_MPU6050_attitude/ESP32_MPU6050_attitude.ino`](firmware/ESP32_MPU6050_attitude/ESP32_MPU6050_attitude.ino)
2. 选择开发板 **NodeMCU-32S**（或 ESP32 Dev Module），选择 COM 口
3. 上传，串口监视器设 115200 波特率

### 查看数据

上电后自动校准（保持传感器静止水平，约 2.5 s），随后输出：

```
Calibrating gyroscope... Keep sensor still!
Bias X: -0.0183 °/s
Bias Y:  0.0241 °/s
Bias Z:  0.0122 °/s
Calibration done.
BLE ready. Output: Time(s),Roll(deg),Pitch(deg)
0.025,74.67,0.60
0.077,91.33,0.27
...
```

BLE 设备名 `ESP32_MPU6050_BLE`，Service `4fafc201-1fb5-459e-8fcc-c5c9c331914b`，Characteristic `beb5483e-36e1-4688-b7f5-ea07361b26a8`（Notify）。

## 数据输出格式 Data Output Format

| 字段 | 格式 | 单位 | 说明 |
| ---- | ---- | ---- | ---- |
| Time | `%.3f` | s | 上电后经过时间 |
| Roll | `%.2f` | deg | 归一化至 (-180, 180] |
| Pitch | `%.2f` | deg | 归一化至 (-180, 180] |

## 项目结构 Project Structure

```
ESP32-MPU6050-Attitude-System/
├── README.md                        # 项目主页（本文件）
├── LICENSE                          # MIT 许可证
├── CHANGELOG.md                     # 固件开发历程（v1~v11）
├── .gitignore
├── firmware/                        # 固件源码
│   ├── ESP32_MPU6050_attitude/      # ★ 最终版主工程（Arduino IDE 直接打开）
│   │   └── ESP32_MPU6050_attitude.ino
│   └── README.md
├── hardware/                        # 硬件资料
│   ├── BOM/                         # 物料清单（xlsx + 订货单）
│   ├── gerber/                      # PCB 制造文件（Gerber 压缩包）
│   ├── pcb/                         # PCB 渲染图 + 立创EDA工程（.epro2）
│   └── datasheets/                  # 芯片数据手册（MPU6050 / NodeMCU-32S / HX9193）
├── data/                            # 测量数据与分析
│   ├── raw/                         # 原始测量 CSV（静态测量 / 单、双障碍物静态模拟）
│   └── analysis/                    # Python 脚本（record_attitude 采集 + plot_attitude 分析）
└── CHANGELOG.md
```

## 固件开发历程 Firmware History

固件经过 11 个版本迭代，当前仓库仅保留最终版主工程；完整演进记录（v1 基础读取 → v5 互补滤波 → v8 静止检测消除漂移 → v11 最终版）见 [`CHANGELOG.md`](CHANGELOG.md)。

**v11（当前）**：校准重试 + 姿态重置 + 低通滤波 + 动态 dt + BLE 传输，位于 `ESP32_MPU6050_attitude/`。

## 测量数据与分析 Measurement & Analysis

- 原始数据：[`data/raw/`](data/raw/)（`Time(s),Raw_Roll,Raw_Pitch,Smooth_Roll,Smooth_Pitch` 五列格式）
- 采集脚本：[`data/analysis/record_attitude.py`](data/analysis/record_attitude.py)（串口读取 + 滑动平均 + 存 CSV）
- 分析脚本：[`data/analysis/plot_attitude.py`](data/analysis/plot_attitude.py)（静态/动态多面板绘图）

```bash
cd data/analysis
python record_attitude.py   # 采集数据（需修改 PORT 为实际串口）
python plot_attitude.py     # 分析绘图（读取 ../raw/ 下数据）
```

## PCB 硬件设计 PCB Design

自研 2 层板（立创EDA）集成 NodeMCU-32S、GY-521、HX9193 LDO、ESD 防护与电源级联：

- PCB 渲染图：[`hardware/pcb/PCB_render_2026-08-03.pdf`](hardware/pcb/PCB_render_2026-08-03.pdf)
- 立创EDA 工程：[`hardware/pcb/ProPrj_esp32mpu6050_finish_improve_copy_2026-08-03.epro2`](hardware/pcb/ProPrj_esp32mpu6050_finish_improve_copy_2026-08-03.epro2)
- Gerber：[`hardware/gerber/Gerber_PCB6finish_2026-07-27.zip`](hardware/gerber/Gerber_PCB6finish_2026-07-27.zip)
- BOM：[`hardware/BOM/`](hardware/BOM/)
- 数据手册：[`hardware/datasheets/`](hardware/datasheets/)

## 许可证 License

本项目采用 **MIT License**，详见 [LICENSE](LICENSE)。

---

**Disclaimer**: This is a research/engineering prototype. Use at your own risk.
