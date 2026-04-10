# IoT System Prototyping 课程项目说明

本仓库用于保存 IoT System Prototyping 课程作业的全部工作文件，包括两个 design challenge、相关实验材料、生成的图表、部署文件、提取后的作业说明材料、notebook、数据集以及报告草稿。

## 仓库结构

- `coursework_assignment/`
  - 课程作业原始 PDF、中文 PDF，以及从 PDF 提取出的纯文本和逐页图片。
- `design-challenge-1/`
  - 基于 Zephyr 的温度监测与控制系统，实现平台为 `nRF54L15 DK`。
- `design-challenge-2/`
  - TinyML 手势识别任务，对应作业中的 `4.1`、`4.2`、`4.3` 和 `Task 5`。
- `labs/`
  - 课程实验参考资料与模板代码。

## Design Challenge 1

`design-challenge-1/` 是一个运行在 `nRF54L15 DK` 上的多线程温度监测系统，使用 `LM335` 模拟温度传感器。

从代码与现有文档可以确认，本项目已经实现了以下核心功能：

- 独立的 ADC 采样线程、逻辑处理线程、串口报告线程、BLE 通信线程、校准线程
- `100 ms` 周期采样
- `60 s` 滚动平均温度计算
- `NORMAL`、`WARNING`、`FAULT`、`DRIFT` 四种系统状态
- 告警 LED 控制逻辑
- BLE 广播平均温度与系统状态
- ADC 故障检测与队列溢出统计
- 通过按键进行实时阈值校准
- 支持完整模式与演示模式的漂移检测

构建命令示例：

> 直接用 VS Code 插件构建即可

```sh
west build -b nrf54l15dk/nrf54l15/cpuapp design-challenge-1
```

更详细的说明见：

- `design-challenge-1/README.md`
- `design-challenge-1/README_CN.md`

## Design Challenge 2

`design-challenge-2/` 现在已经覆盖本仓库中实现的全部 TinyML 任务：

- `4.1 Baseline`
- `4.2 Architecture Exploration`
- `4.3 Personalisation and Stability Analysis`
- `5 Extending the System with a New Gesture Class`

仓库中已经包含以下成果文件：

- 用于 baseline、架构探索、个性化和类别扩展的 Colab notebook
- 多个 CNN 架构导出的 `.cc` 模型文件
- 已加入 MCU 端延迟测量代码的 Arduino `magic_wand` 工程
- 多个候选模型的板端 latency 日志
- accuracy、model size、latency、个性化结果和类别扩展结果的汇总 CSV
- 生成好的图表：
  - `accuracy_vs_model_size.png`
  - `accuracy_vs_mcu_latency.png`
  - `task_5_confusion_matrix.png`
- 中英文报告材料

关键文件包括：

- `design-challenge-2/notebook/4.2.ipynb`
- `design-challenge-2/notebook/4.3.ipynb`
- `design-challenge-2/notebook/5.ipynb`
- `design-challenge-2/mcu_latency_template.csv`
- `design-challenge-2/notebook/task_4_3_comparison.csv`
- `design-challenge-2/notebook/task_5_summary.csv`
- `design-challenge-2/document/report_4_3_final.md`
- `design-challenge-2/document/report_5_final.md`
- `design-challenge-2/magic_wand/magic_wand.ino`

### 任务 4.2 概况

`4.2` 主要探索至少五种 CNN 架构，并比较以下指标：

- PC 端分类准确率
- 量化后模型大小
- MCU 推理延迟

仓库中已经包含训练结果、trade-off 图表、部署用 `.cc` 导出文件以及支持报告撰写的总结材料。

### 任务 4.3 概况

对于第 `15` 组，个人数字是 `5` 和 `6`。

`4.3` 的最终结果如下：

| 模型 | 原始测试集 PC 准确率 (%) | 个性化测试集 PC 准确率 (%) | 原始测试集量化准确率 (%) | 个性化测试集量化准确率 (%) | 个人重复手势准确率 (%) | 个人一致性 (%) |
|---|---:|---:|---:|---:|---:|---:|
| baseline_original | 93.73 | 91.96 | 93.82 | 91.87 | 52.17 | 53.08 |
| baseline_personalised | 91.91 | 90.65 | 91.82 | 90.30 | 95.65 | 96.16 |

这说明只加入少量用户个人数据，就能显著提升该用户自己手势的识别准确率和稳定性，而对原始共享数据集的性能影响很小。

### 任务 5 概况

对于第 `15` 组，新增类别字母是 `P`。

`Task 5` 的最终结果如下：

| 指标 | 数值 |
|---|---:|
| Baseline 数字 PC 准确率 (%) | 96.00 |
| 扩展模型数字 PC 准确率 (%) | 94.00 |
| 扩展模型 11 类整体 PC 准确率 (%) | 94.17 |
| 扩展模型字母 `P` 测试准确率 (%) | 100.00 |
| 扩展模型重复字母准确率 (%) | 90.00 |
| 扩展模型字母一致性 (%) | 90.00 |

混淆分析显示，测试集中的 `P` 没有和任何数字发生混淆，因此新增类别的识别效果是很干净的。加入 `P` 的主要代价，是原有数字识别准确率小幅下降了 `2.00` 个百分点。

## 完成度核对结论

根据当前仓库内容、提取后的作业文本、notebook、结果表和报告文件，可以得出以下结论：

- `Design Challenge 1`：已经完成主要要求，并且连扩展项 `Live Calibration` 与 `Drift Detection` 也已经实现，整体完成度很高。
- `Design Challenge 2` 的 `4.1-4.2`：已经完成，仓库中具备 baseline 成果、至少 5 个模型架构、accuracy 与量化模型大小记录、MCU latency 测试、结果图表和报告结论。
- `Design Challenge 2` 的 `4.3`：已经完成，包含数字 `5` 和 `6` 的个人数据实验、对比指标和最终报告。
- `Design Challenge 2` 的 `Task 5`：已经完成，包含新增类别 `P`、训练后的 `11 类` 模型、混淆分析、一致性指标和最终报告。

换句话说，如果当前要判断本仓库是否已经覆盖：

- `Design Challenge 1` 全部要求
- `Design Challenge 2` 的 `4.1-4.3`
- `Design Challenge 2` 的 `Task 5`

那么结论是：**基本可以认为已经完成，并且已经接近可提交状态。**

## 仍需注意的地方

后续最值得继续打磨的，不再是“有没有实现”，而是“最终提交材料是否组织得足够清楚”。例如：

- 报告是否严格对应评分点和 rubric 用语
- 图表、表格与实验结论是否在正文中被清楚引用
- Arduino 部署部分是否明确说明了模型切换和标签映射更新
- 每位组员的个人贡献是否按课程要求写明
- 是否补充了 build system 给出的 memory footprint 等证据
