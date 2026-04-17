# Design Challenge 1

本文档与 `README.md` 同步维护。后续如有更新，应同时修改英文版和中文版，避免内容不一致。

## 项目概述

本应用基于 nRF54L15 DK 和 LM335 模拟温度传感器，实现了 Design Challenge 1 的核心要求与加分项。

LM335 连接方式与 Lab 2 一致：

- `LM335 V+` 接 `P1.11 / AIN4`
- `LM335 V+` 通过 `330 ohm` 电阻接 `VDD (3.3V)`
- `LM335 GND` 接开发板 `GND`
- `LM335 ADJ` 悬空

![image-20260417141829748](https://media.opennet.top/i/2026/04/17/z16w0p-0.png)

## 已实现功能

- 至少四个独立线程：
  - ADC 采集线程
  - 逻辑处理线程
  - 串口报告线程
  - BLE 通信线程
- 额外增加一个校准线程：
  - 处理按钮输入与实时阈值调整
- 使用内核定时器触发 `100 ms` 精确定时采样
- 对最近 `600` 个样本执行 1 分钟滚动平均
- 实现确定性的 `NORMAL / WARNING / FAULT / DRIFT` 状态机
- `WARNING` 与 `DRIFT` 状态下 LED 执行 `50 ms` 脉冲闪烁
- `FAULT` 状态下 LED 常亮
- BLE 广播中携带平均温度与系统状态
- 对 ADC 转换失败和超范围电压进行故障检测
- 支持实时阈值校准
- 支持基于 Welford 在线算法的漂移检测，无需存储 24 小时原始数据

## 线程结构

- `adc_sampling`
  - 等待 `100 ms` 定时信号
  - 读取 ADC
  - 校验电压是否合法
  - 将样本送入消息队列
- `logic_engine`
  - 消费采样数据
  - 更新 1 分钟滚动平均
  - 执行状态机
  - 控制 LED 告警行为
  - 进行分钟级漂移统计
- `reporting`
  - 每秒输出一次结构化 UART 日志
- `communication`
  - 每秒更新一次 BLE 广播
  - `FAULT` 状态下停止广播
- `calibration`
  - 接收按钮事件
  - 安全更新告警阈值
  - 切换漂移检测模式

## 可靠性设计

- 采样路径与串口/BLE 解耦，降低采样抖动
- 线程间通信使用有界消息队列
- 共享快照使用 mutex 保护
- ADC 错误计数与队列溢出计数会打印到串口日志
- 1 分钟温度历史使用整数 centi-degree 存储，降低 SRAM 占用
- 漂移检测使用 Welford 在线统计，只保留统计量，不保留长时间原始数组

## BLE 载荷格式

Manufacturer Specific Data 包含：

- `int16` 平均温度，单位为摄氏度的百分之一，采用 big-endian
- `uint8` 系统状态：
  - `0 = NORMAL`
  - `1 = WARNING`
  - `2 = FAULT`
  - `3 = DRIFT`

## 实时校准按键

- `SW0`：告警阈值降低 `0.50 C`
- `SW1`：告警阈值升高 `0.50 C`
- `SW2`：阈值恢复到 `28.00 C`，并重置漂移跟踪
- `SW3`：切换漂移检测模式
  - `24h` 模式：要求 `1440` 个受控分钟
  - `demo` 模式：要求 `3` 个受控分钟，便于快速测试

## 构建

构建前，先在 Nordic Board Configurator 中将 ADC 参考电压设为 `3.3V`，这是 Lab 2 的前提条件。

> 直接用VS Code的构建功能就可以 以下忽略

示例构建命令：

```sh
west build -b nrf54l15dk/nrf54l15/cpuapp design-challenge-1
```

烧录：

```sh
west flash
```

## 串口输出示例

```text
[12000 ms] Avg: 22.11 C | Raw: 22.25 C | MV: 2954 | Thr: 28.00 C | Mode: NORMAL | LED: OFF | DriftRef: *0.00 C | LastMin: 22.09 C | Ctrl: 2/1440 | Env: CTRL | ADC_ERR: 0 | Q_OVR: 0
```

说明：

- `Avg`：当前 1 分钟滚动平均温度
- `Raw`：当前最新样本温度
- `MV`：LM335 原始输出毫伏值
- `Thr`：当前 WARNING 阈值
- `Mode`：系统状态
- `DriftRef`：漂移参考基线
  - `*` 表示基线尚未锁定
  - `D:` 表示当前处于 demo 漂移模式
- `LastMin`：上一分钟的分钟平均值
- `Ctrl`：已累计的受控分钟数 / 建立漂移基线所需分钟数
- `Env`：
  - `CTRL` 表示当前分钟环境稳定，适合作为漂移分析样本
  - `TRANS` 表示当前分钟波动较大，不用于漂移基线判断
- `ADC_ERR`：ADC 错误计数
- `Q_OVR`：消息队列溢出计数

## 测试方法

### 1. 测试实时校准

1. 上电并打开串口，确认日志中显示 `Thr: 28.00 C`
2. 按 `SW0`（BUTTON 0），串口应打印 `Calibration: threshold -> ...`
3. 多按几次 `SW0`，把阈值降到当前室温以下
4. 当 1 分钟平均值超过新阈值后，模式应切换为 `WARNING`，LED 应闪烁
5. 按 `SW1` 提高阈值，模式应返回 `NORMAL`
6. 按 `SW2`，阈值恢复为默认值

### 2. 测试漂移检测

1. 按一次 `SW3`，进入 demo 模式
2. 串口应打印：

```text
Drift mode -> demo (3 controlled minute(s) required)
```

3. 将传感器放在稳定环境中，保持约 `3` 分钟不动
4. 串口应出现：

```text
Drift baseline locked at ... [demo]
```

5. 基线建立后，加热传感器，并让温度相对基线偏移超过 `2.0 C`
6. 尽量保持这一偏移至少 `1` 分钟
7. 系统模式应切换为 `DRIFT`
8. BLE 广播中的状态字节也应变为 `3`
9. 温度恢复接近基线后，系统应退出 `DRIFT`
10. 按 `SW3` 可以切回真实 `24h` 模式，按 `SW2` 可以重置漂移统计

### 3. 测试故障处理

1. 临时断开 LM335 输出线或地线
2. 串口日志应进入 `FAULT`
3. `ADC_ERR` 计数应增加
4. LED 应常亮
5. BLE 广播应停止
