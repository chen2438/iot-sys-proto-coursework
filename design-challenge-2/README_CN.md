# Design Challenge 2 说明

本目录保存 IoT System Prototyping 课程作业中 `Design Challenge 2` 的相关材料。

当前已经覆盖：

- `4.1 Baseline`
- `4.2 Architecture Exploration`
- `4.3 Personalisation and Stability Analysis`
- `5 Extending the System with a New Letter Class`

## 目录结构

- `notebook/`
  - 用于 Google Colab 的 notebook，包括 baseline、架构探索和个性化实验。
- `dataset/`
  - 第 `15` 组对应的个人数据集，即数字 `5`、`6` 以及字母 `P`。
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

## 任务 5

`Task 5` 要求把原来的 `10` 类数字识别器扩展成 `11` 类，在数字 `0-9` 之外加入小组对应的新字母类别。

对于第 `15` 组，分配到的字母是：

- `P`

对应数据文件：

- `dataset/wanddata_P.json`

仓库中现在已经补好了可直接在 Colab 运行的 `Task 5` 工作流，它会：

- 重新构建 baseline 的 `10 类` 数字模型
- 训练包含 `P` 的 `11 类` 扩展模型
- 导出量化 `.tflite` 模型和部署用 `.cc` 数组
- 评估新增字母后对原有数字准确率的影响
- 计算字母 `P` 的识别效果和一致性指标
- 生成可直接用于报告的 confusion matrix 结果

主要文件：

- `notebook/5.ipynb`
- `notebook/5_saved.ipynb`
- `notebook/task_5_summary.csv`
- `document/report_5_final.md`

在 Colab 跑完 `notebook/5.ipynb` 之后，预期会得到这些结果文件：

- `task_5_summary.csv`
- `task_5_letter_consistency.csv`
- `task_5_confusion_matrix.csv`
- `task_5_p_confusions.csv`
- `task_5_confusion_matrix.png`

### Task 5 最终结果摘要

| 指标 | 数值 |
|---|---:|
| Baseline 数字 PC 准确率 (%) | 96.00 |
| 扩展模型数字 PC 准确率 (%) | 94.00 |
| 扩展模型 11 类整体 PC 准确率 (%) | 94.17 |
| 扩展模型字母 `P` 测试准确率 (%) | 100.00 |
| 扩展模型重复字母准确率 (%) | 90.00 |
| 扩展模型字母一致性 (%) | 90.00 |

这说明加入新类别 `P` 之后，原有数字识别准确率只小幅下降，但新字母本身的识别效果和稳定性都很好。

部署提醒：

- 如果后续要把新的 `11 类` 模型切到 Arduino 工程里，还需要同步修改 [magic_wand.ino](/Users/nanmener/Sync/UoB/TB2/03_IoT_System_Prototyping/iot-sys-proto-coursework/design-challenge-2/magic_wand/magic_wand.ino)，把 `label_count` 和标签数组一起扩展到包含 `P`

## 总结

`4.2` 展示了模型架构复杂度与嵌入式部署成本之间的关系，`4.3` 展示了个性化数据对用户特定手势识别稳定性的巨大提升，而 `5` 则进一步验证了系统在新增类别后的可扩展性。这三部分结合起来，构成了对 Arduino Nano 33 BLE Sense 上 TinyML 手势识别系统较完整的评估。
