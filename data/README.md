# 测量数据与数据分析 (Measurement Data & Analysis)

## 目录说明

| 目录 | 内容 |
| ---- | ---- |
| [`raw/`](raw/) | 原始测量数据（CSV，五列格式） |
| [`analysis/`](analysis/) | Python 脚本：串口采集 + 数据分析绘图 |

## 原始数据格式

CSV 文件为五列格式：

| 列名 | 含义 | 单位 |
| ---- | ---- | ---- |
| `Time(s)` | 采样时间 | s |
| `Raw_Roll` | 横滚角原始值 | deg |
| `Raw_Pitch` | 俯仰角原始值 | deg |
| `Smooth_Roll` | 滑动平均后的横滚角 | deg |
| `Smooth_Pitch` | 滑动平均后的俯仰角 | deg |

## 现有数据文件 (raw/)

| 文件 | 说明 | 行数 |
| ---- | ---- | ---- |
| `静态测量.csv` | 静态验收测量（Roll/Pitch 稳定性） | 21,613 |
| `单障碍物静态模拟.csv` | 单障碍物场景静态模拟 | 10,110 |
| `双障碍物静态模拟.csv` | 双障碍物场景静态模拟 | 7,451 |

## Python 脚本 (analysis/)

| 脚本 | 功能 |
| ---- | ---- |
| [`record_attitude.py`](analysis/record_attitude.py) | 串口读取 ESP32 姿态数据，滑动平均平滑，保存 CSV（依赖 `pyserial`） |
| [`plot_attitude.py`](analysis/plot_attitude.py) | 静态/动态测量数据多面板绘图（时间序列 + ±1° 容限带 + 直方图 + 箱线图） |

### 使用示例

```bash
# 1. 采集数据（先改脚本内 PORT 为实际串口号，关闭 Arduino IDE 串口监视器）
cd data/analysis
python record_attitude.py

# 2. 分析绘图（读取 ../raw/ 下数据，输出到 plots/ 目录）
python plot_attitude.py
```

> 脚本均需在 `data/analysis/` 目录下运行；依赖 `pyserial`、`pandas`、`numpy`、`matplotlib`。
