# 固件 (Firmware)

ESP32 + MPU6050 姿态采集系统的固件源码。

## 目录

```
firmware/
└── ESP32_MPU6050_attitude/   ★ 最终版主工程（Arduino IDE 直接打开）
    └── ESP32_MPU6050_attitude.ino
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

当前为 **v11 最终版**：互补滤波 + 静止检测 + 零偏校准重试 + 低通滤波 + 动态 dt + BLE 传输。

完整演进记录（v1~v11）见 [`../CHANGELOG.md`](../CHANGELOG.md)。
