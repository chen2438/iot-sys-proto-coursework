# IoT System Prototyping 课程项目说明

本仓库用于保存 IoT System Prototyping 课程作业的全部工作文件，包括两个 design challenge、相关实验材料、生成的图表、部署文件以及报告草稿。

## 仓库结构

- `coursework_assignment/`
  - 课程作业原始 PDF、中文 PDF，以及从 PDF 提取出的纯文本和逐页图片。
- `design-challenge-1/`
  - 基于 Zephyr 的温度监测与控制系统，实现平台为 `nRF54L15 DK`。
- `design-challenge-2/`
  - TinyML 手势识别任务，对应作业中的 `4.1` 与 `4.2` 部分。
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

`design-challenge-2/` 当前覆盖课程作业中的：

- `4.1 Baseline`
- `4.2 Architecture Exploration`

仓库中已经包含以下成果文件：

- 用于 Colab 的 notebook
- 多个 CNN 架构导出的 `.cc` 模型文件
- 已加入 MCU 端延迟测量代码的 Arduino `magic_wand` 工程
- 多个候选模型的板端 latency 日志
- accuracy、model size、latency 的汇总 CSV
- 生成好的图表：
  - `accuracy_vs_model_size.png`
  - `accuracy_vs_mcu_latency.png`
- 中英文报告草稿

关键文件包括：

- `design-challenge-2/notebook/4.2.ipynb`
- `design-challenge-2/mcu_latency_template.csv`
- `design-challenge-2/magic_wand/magic_wand.ino`
- `design-challenge-2/document/report_4_1_4_2_draft_final.md`

## 完成度核对结论

根据当前仓库内容，以及对课程 PDF 的文本核对，可以得出以下结论：

- `Design Challenge 1`：已经完成主要要求，并且连扩展项 `Live Calibration` 与 `Drift Detection` 也已经实现，整体完成度很高。
- `Design Challenge 2` 的 `4.1-4.2`：已经完成。仓库中已经具备 baseline 运行成果、至少 5 个模型架构、accuracy 与量化模型大小记录、MCU latency 测试、结果图表和报告草稿。

换句话说，如果当前要判断本仓库是否已经覆盖：

- `Design Challenge 1` 全部要求
- `Design Challenge 2` 的 `4.1-4.2`

那么结论是：**基本可以认为已经完成，并且已经接近可提交状态。**

## 仍需注意的地方

后续最值得继续打磨的，不再是“有没有实现”，而是“最终提交材料是否组织得足够清楚”。例如：

- 报告是否严格对应评分点
- 图表与实验结论是否表述清晰
- 每位组员的个人贡献是否写明
- 是否补充了 build system 给出的 memory footprint 等证据

