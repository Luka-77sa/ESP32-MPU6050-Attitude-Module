# 固件 (Firmware)

ESP32 + MPU6050 姿态采集系统的全部固件源码。

## 目录

```
firmware/
├── ESP32_MPU6050_attitude/   ★ 最终版主工程（Arduino IDE 直接打开）
│   └── ESP32_MPU6050_attitude.ino
└── archive/                  历史版本归档（v1~v10 功能演进）
    ├── README.md             版本说明
    └── 01_basic_read.ino ~ 10_ble_v2.ino
```

## 开发环境

- **IDE**: Arduino IDE ≥ 1.8.10
- **板级包**: ESP32 by Espressif Systems（Boards Manager 安装）
- **依赖库**: `MPU6050`（Library Manager）、`Wire`、BLE 系列（板级包内置）

## 烧录步骤

1. 打开 `ESP32_MPU6050_attitude/ESP32_MPU6050_attitude.ino`
2. 选择开发板 **NodeMCU-32S** 或 ESP32 Dev Module
3. 选择 COM 口，上传
4. 串口监视器 115200 波特率查看输出

## 版本说明

| 位置 | 版本 | 功能 |
| ---- | ---- | ---- |
| `archive/01~04` | v1~v4 | 基础读取 → 物理单位 → 串口绘图 |
| `archive/05~08` | v5~v8 | 互补滤波 → 零偏校准 → 静止检测消除静态漂移 |
| `archive/09~10` | v9~v10 | BLE 无线传输 v1/v2 |
| `ESP32_MPU6050_attitude/` | **v11** | 最终版：全部功能 + 校准重试 + 姿态重置 + 动态 dt |

详细演进记录见 [`../CHANGELOG.md`](../CHANGELOG.md)。
