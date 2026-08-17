# -*- coding: utf-8 -*-
"""
姿态数据静态与动态绘图脚本
读取两个 CSV 文件（位于 ../raw/ 目录）：
  - 静态测量.csv
  - 单障碍物静态模拟.csv
生成两组图表并保存为 PNG 文件到 ./plots 目录
用法（在 data/analysis/ 目录下执行）：
  python plot_attitude.py
"""

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import matplotlib
import os

# ============================================================
# 解决中文乱码（适用于 Windows / macOS / Linux）
# ============================================================
matplotlib.rcParams['font.sans-serif'] = [
    'SimHei',               # Windows 黑体
    'Microsoft YaHei',      # Windows 微软雅黑
    'PingFang SC',          # macOS 苹方
    'Noto Sans CJK SC',     # Linux 思源黑体
    'WenQuanYi Zen Hei'     # Linux 文泉驿
]
matplotlib.rcParams['axes.unicode_minus'] = False   # 修复负号显示

# ============================================================
# 配置
# ============================================================
STATIC_FILE = '静态测量.csv'
DYNAMIC_FILE = '单障碍物静态模拟.csv'
OUTPUT_DIR = 'plots'

# 若脚本与数据不在同一目录，可在此指定 raw 数据路径，例如：
# DATA_DIR = os.path.join(os.path.dirname(__file__), '..', 'raw')
# STATIC_FILE = os.path.join(DATA_DIR, '静态测量.csv')
# DYNAMIC_FILE = os.path.join(DATA_DIR, '单障碍物静态模拟.csv')

os.makedirs(OUTPUT_DIR, exist_ok=True)

# ============================================================
# 读取数据
# ============================================================
static_df = pd.read_csv(STATIC_FILE)
dynamic_df = pd.read_csv(DYNAMIC_FILE)

time_static = static_df['Time(s)']
roll_static = static_df['Smooth_Roll']
pitch_static = static_df['Smooth_Pitch']

time_dynamic = dynamic_df['Time(s)']
roll_dynamic = dynamic_df['Smooth_Roll']
pitch_dynamic = dynamic_df['Smooth_Pitch']

# ============================================================
# 静态数据绘图（4 个子图）
# ============================================================
def plot_static(time, roll, pitch):
    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle('静态测量数据分析 (Static Measurement)', fontsize=16, fontweight='bold')

    # 1. 时间序列 + 均值线 + ±1° 容限带
    ax1 = axes[0, 0]
    ax1.plot(time, roll, label='Roll', color='blue', linewidth=1.2)
    ax1.plot(time, pitch, label='Pitch', color='red', linewidth=1.2)
    roll_mean = roll.mean()
    pitch_mean = pitch.mean()
    ax1.axhline(y=roll_mean, color='blue', linestyle='--', linewidth=1, label=f'Roll mean = {roll_mean:.3f}°')
    ax1.axhline(y=pitch_mean, color='red', linestyle='--', linewidth=1, label=f'Pitch mean = {pitch_mean:.3f}°')
    ax1.fill_between(time, roll_mean - 1, roll_mean + 1, color='blue', alpha=0.08)
    ax1.fill_between(time, pitch_mean - 1, pitch_mean + 1, color='red', alpha=0.08)
    ax1.set_xlabel('时间 (s)')
    ax1.set_ylabel('角度 (deg)')
    ax1.set_title('(a) 时间序列与 ±1° 容限带')
    ax1.legend(loc='upper right', fontsize=9)
    ax1.grid(True, alpha=0.3)

    # 2. 偏差图
    ax2 = axes[0, 1]
    ax2.plot(time, roll - roll_mean, label='Roll 偏差', color='blue', linewidth=1.2)
    ax2.plot(time, pitch - pitch_mean, label='Pitch 偏差', color='red', linewidth=1.2)
    ax2.axhline(y=0, color='black', linestyle='-', linewidth=0.8)
    ax2.axhline(y=1, color='gray', linestyle='--', linewidth=0.5)
    ax2.axhline(y=-1, color='gray', linestyle='--', linewidth=0.5)
    ax2.set_xlabel('时间 (s)')
    ax2.set_ylabel('相对均值的偏差 (deg)')
    ax2.set_title('(b) 偏差时间序列')
    ax2.legend(loc='upper right', fontsize=9)
    ax2.grid(True, alpha=0.3)

    # 3. 直方图
    ax3 = axes[1, 0]
    bins = 30
    ax3.hist(roll, bins=bins, alpha=0.6, color='blue', edgecolor='black', label='Roll')
    ax3.hist(pitch, bins=bins, alpha=0.6, color='red', edgecolor='black', label='Pitch')
    ax3.axvline(x=roll_mean, color='blue', linestyle='--', linewidth=2, label='Roll 均值')
    ax3.axvline(x=pitch_mean, color='red', linestyle='--', linewidth=2, label='Pitch 均值')
    ax3.set_xlabel('角度 (deg)')
    ax3.set_ylabel('频数')
    ax3.set_title('(c) 角度分布直方图')
    ax3.legend(loc='upper right', fontsize=9)
    ax3.grid(True, alpha=0.3)

    # 4. 箱线图
    ax4 = axes[1, 1]
    box_data = [roll, pitch]
    bp = ax4.boxplot(box_data, patch_artist=True,
                     boxprops=dict(facecolor='lightblue', color='blue'),
                     whiskerprops=dict(color='blue'),
                     capprops=dict(color='blue'),
                     medianprops=dict(color='red', linewidth=2))
    ax4.set_xticklabels(['Roll', 'Pitch'])
    # 标注均值点
    for i, data in enumerate(box_data, start=1):
        mean_val = data.mean()
        ax4.scatter(i, mean_val, marker='D', color='darkred', s=60, zorder=5, label='均值' if i==1 else "")
    ax4.axhline(y=0, color='gray', linestyle='--', linewidth=0.8)
    ax4.set_ylabel('角度 (deg)')
    ax4.set_title('(d) 箱线图 (含均值点)')
    ax4.grid(True, alpha=0.3, axis='y')
    ax4.legend(loc='upper right', fontsize=9)

    plt.tight_layout()
    plt.subplots_adjust(top=0.93)
    return fig

# ============================================================
# 动态数据绘图（2 个子图：完整序列 + 动态段放大）
# ============================================================
def plot_dynamic(time, roll, pitch):
    fig, axes = plt.subplots(2, 1, figsize=(14, 10))
    fig.suptitle('动态测量数据分析 (Dynamic Measurement)', fontsize=16, fontweight='bold')

    # 1. 完整时间序列
    ax1 = axes[0]
    ax1.plot(time, roll, label='Roll', color='blue', linewidth=1.2)
    ax1.plot(time, pitch, label='Pitch', color='red', linewidth=1.2)
    ax1.set_xlabel('时间 (s)')
    ax1.set_ylabel('角度 (deg)')
    ax1.set_title('(a) 完整姿态时间序列')
    ax1.legend(loc='upper right')
    ax1.grid(True, alpha=0.3)

    # 2. 自动截取动态变化最剧烈的区间（放大视图）
    window = 50
    roll_var = roll.rolling(window).var().fillna(0)
    pitch_var = pitch.rolling(window).var().fillna(0)
    total_var = roll_var + pitch_var
    threshold = total_var.quantile(0.95)
    high_var_mask = total_var > threshold

    ax2 = axes[1]
    if high_var_mask.any():
        indices = np.where(high_var_mask)[0]
        start_idx = indices[0]
        end_idx = indices[-1]
        # 适当扩展边界
        start_idx = max(0, start_idx - 10)
        end_idx = min(len(time), end_idx + 10)
        time_sub = time.iloc[start_idx:end_idx]
        roll_sub = roll.iloc[start_idx:end_idx]
        pitch_sub = pitch.iloc[start_idx:end_idx]

        ax2.plot(time_sub, roll_sub, label='Roll (放大)', color='blue', linewidth=1.5)
        ax2.plot(time_sub, pitch_sub, label='Pitch (放大)', color='red', linewidth=1.5)
        ax2.set_title('(b) 动态变化放大视图 (自动截取)')
    else:
        ax2.text(0.5, 0.5, '无显著动态变化', ha='center', va='center', transform=ax2.transAxes, fontsize=14)
        ax2.set_title('(b) 动态段放大视图')
    ax2.set_xlabel('时间 (s)')
    ax2.set_ylabel('角度 (deg)')
    ax2.legend(loc='upper right')
    ax2.grid(True, alpha=0.3)

    plt.tight_layout()
    plt.subplots_adjust(top=0.93)
    return fig

# ============================================================
# 生成并保存图像
# ============================================================
fig_static = plot_static(time_static, roll_static, pitch_static)
fig_static.savefig(os.path.join(OUTPUT_DIR, 'static_analysis.png'), dpi=300, bbox_inches='tight')
plt.close(fig_static)

fig_dynamic = plot_dynamic(time_dynamic, roll_dynamic, pitch_dynamic)
fig_dynamic.savefig(os.path.join(OUTPUT_DIR, 'dynamic_analysis.png'), dpi=300, bbox_inches='tight')
plt.close(fig_dynamic)

print(f"✅ 图像已成功保存至 '{OUTPUT_DIR}' 文件夹。")
print(f"   - 静态分析图: {OUTPUT_DIR}/static_analysis.png")
print(f"   - 动态分析图: {OUTPUT_DIR}/dynamic_analysis.png")