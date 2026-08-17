# -*- coding: utf-8 -*-
"""
文件名: record_attitude.py
功能: 读取 ESP32 串口姿态数据（Roll/Pitch 格式），滑动平均平滑，保存 CSV
用法:
  1. 确认 ESP32 已烧录互补滤波代码并正在输出数据
  2. 关闭 Arduino IDE 串口监视器
  3. 修改 PORT 为实际串口号
  4. 运行本脚本，按 Ctrl+C 停止
"""

import serial
import time
import csv
import os
import re
from datetime import datetime

# ============================================================
# 用户配置（请根据实际情况修改）
# ============================================================
PORT = 'COM5'                # ESP32 的串口号
BAUDRATE = 115200            # 波特率
OUTPUT_DIR = 'data'          # 数据保存文件夹
FILTER_WINDOW = 5            # 滑动平均窗口大小 (推荐 3~10)

# ============================================================
# 初始化
# ============================================================

# 创建数据文件夹
if not os.path.exists(OUTPUT_DIR):
    os.makedirs(OUTPUT_DIR)

# 生成带时间戳的文件名
timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
filename = f"{OUTPUT_DIR}/attitude_{timestamp}.csv"
print(f"📁 数据将保存到: {filename}")

# 打开串口
try:
    ser = serial.Serial(PORT, BAUDRATE, timeout=1)
    print(f"✅ 成功连接到串口 {PORT}")
except Exception as e:
    print(f"❌ 无法打开串口 {PORT}")
    print(f"   错误详情: {e}")
    print("   💡 请检查:")
    print("      1. 端口号是否正确")
    print("      2. 是否已关闭 Arduino 的串口监视器")
    exit()

# 打开 CSV 文件
csv_file = open(filename, 'w', newline='', encoding='utf-8')
writer = csv.writer(csv_file)
writer.writerow(['Time(s)', 'Raw_Roll', 'Raw_Pitch', 'Smooth_Roll', 'Smooth_Pitch'])
csv_file.flush()

print("📊 开始记录数据... (按 Ctrl+C 停止)")
print("📌 滑动平均窗口大小:", FILTER_WINDOW)
print("⏳ 等待 ESP32 启动...")
time.sleep(3)
print("-" * 60)

# ============================================================
# 滑动平均滤波器
# ============================================================
roll_history = []
pitch_history = []

def smooth_value(new_value, history, window_size):
    history.append(new_value)
    if len(history) > window_size:
        history.pop(0)
    return sum(history) / len(history)

def is_valid_data(line):
    """检查是否为有效的姿态数据行"""
    pattern = r'^Roll:[-+]?\d+\.\d+,Pitch:[-+]?\d+\.\d+$'
    return re.match(pattern, line) is not None

# ============================================================
# 主循环
# ============================================================
start_time = time.time()
frame_count = 0
skip_count = 0

try:
    while True:
        raw_line = ser.readline()
        if not raw_line:
            continue

        line = raw_line.decode('utf-8').strip()
        if not line:
            continue

        # 跳过非数据行（如启动信息）
        if not is_valid_data(line):
            skip_count += 1
            if skip_count <= 5:
                print(f"⏭️ 跳过: {line}")
            continue

        # 解析数据
        parts = line.split(',')
        if len(parts) != 2:
            continue

        try:
            raw_roll = float(parts[0].split(':')[1])
            raw_pitch = float(parts[1].split(':')[1])
        except ValueError:
            continue

        # 平滑
        smooth_roll = smooth_value(raw_roll, roll_history, FILTER_WINDOW)
        smooth_pitch = smooth_value(raw_pitch, pitch_history, FILTER_WINDOW)

        elapsed = time.time() - start_time
        frame_count += 1

        # 写入 CSV
        writer.writerow([
            f"{elapsed:.3f}",
            f"{raw_roll:.2f}",
            f"{raw_pitch:.2f}",
            f"{smooth_roll:.2f}",
            f"{smooth_pitch:.2f}"
        ])
        csv_file.flush()

        # 屏幕显示
        if frame_count <= 15:
            print(f"[{frame_count:4d}]  T={elapsed:.2f}s  Roll={smooth_roll:7.2f}°  Pitch={smooth_pitch:7.2f}°")
        elif frame_count % 30 == 0:
            print(f"📈 已记录 {frame_count} 帧")

except KeyboardInterrupt:
    print("\n🛑 用户手动停止")
except Exception as e:
    print(f"\n⚠️ 发生意外错误: {e}")
finally:
    ser.close()
    csv_file.close()
    print("-" * 60)
    print(f"📊 总计记录: {frame_count} 帧")
    print(f"⏭️ 跳过非数据行: {skip_count} 行")
    print(f"📁 数据已保存到: {filename}")
    print("🔌 串口已关闭")