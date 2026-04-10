# Design Challenge 2 说明

本目录保存 IoT System Prototyping 课程作业中 `Design Challenge 2` 的相关材料。

当前已经覆盖：

- `4.1 Baseline`
- `4.2 Architecture Exploration`
- `4.3 Personalisation and Stability Analysis`

## 目录结构

- `notebook/`
  - 用于 Google Colab 的 notebook，包括 baseline、架构探索和个性化实验。
- `dataset/`
  - 第 `15` 组对应的个人数据集，即数字 `5` 和 `6`。
- `models/`
  - 导出的 `.cc` 模型文件，可用于 Arduino / TensorFlow Lite Micro 部署。
- `latency_log/`
  - `4.2` 选定模型的 MCU 延迟日志。
- `document/`
  - 报告草稿和补充说明材料。

## 任务 4.1

`4.1` 完成了提供的 Magic Wand baseline 流程，使用原始数字数据集以及题目给定的 train / validation / test 划分比例。该部分是后续任务的前提，但本身不计分。

参考 notebook：

- `notebook/Baseline.ipynb`

## 任务 4.2

`4.2` 主要探索不同 CNN 架构在以下三项指标之间的 trade-off：

- 分类准确率
- 量化后模型大小
- MCU 推理延迟

仓库中已经包含：

- 多种架构训练结果
- 导出的 `.cc` 模型文件
- `accuracy_vs_model_size.png`
- `accuracy_vs_mcu_latency.png`
- MCU 端 latency 记录
- 可直接用于报告的总结材料

主要文件：

- `notebook/4.2.ipynb`
- `mcu_latency_template.csv`
- `document/report_4_1_4_2_draft_final.md`

## 任务 4.3

`4.3` 用于研究加入个人数据后，对准确率和预测稳定性的影响。

对于第 `15` 组，个人数字为：

- `5`
- `6`

对应个人数据文件：

- `dataset/wanddata_5.json`
- `dataset/wanddata_6.json`

该实验比较：

- 仅用原始数据训练的 baseline 模型
- 将个人数据混入原始数据后训练得到的 personalised 模型

### 4.3 最终结果摘要

| 模型 | 原始测试集 PC 准确率 (%) | 个性化测试集 PC 准确率 (%) | 原始测试集量化准确率 (%) | 个性化测试集量化准确率 (%) | 个人重复手势准确率 (%) | 个人一致性 (%) |
|---|---:|---:|---:|---:|---:|---:|
| baseline_original | 93.73 | 91.96 | 93.82 | 91.87 | 52.17 | 53.08 |
| baseline_personalised | 91.91 | 90.65 | 91.82 | 90.30 | 95.65 | 96.16 |

这说明加入个人数据后，模型在原始共享数据集上的表现只略有下降，但对用户本人重复手势的识别准确率和稳定性都得到了非常明显的提升。

主要文件：

- `notebook/4.3.ipynb`
- `notebook/task_4_3_comparison.csv`
- `document/report_4_3_final.md`

## 总结

`4.2` 展示了模型架构复杂度与嵌入式部署成本之间的关系，`4.3` 则展示了个性化数据对用户特定手势识别稳定性的巨大提升。两部分结合起来，构成了对 Arduino Nano 33 BLE Sense 上 TinyML 手势识别系统较完整的评估。
