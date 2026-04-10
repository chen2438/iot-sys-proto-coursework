# Design Challenge 2 - 任务 4.1-4.2

本目录当前先覆盖 `CW_EEMEM0018_IoTSystemPrototyping_V3.pdf` 中的 `4.1 Baseline` 和 `4.2 Architecture Exploration`。

## 当前文件

- `notebook/4.2.ipynb`
  - 面向 Google Colab 的 notebook。
  - 自动下载 baseline 使用的 digits 数据集。
  - 训练多种 CNN 架构。
  - 统计 PC 端测试准确率、量化后 `.tflite` 模型大小。
  - 生成 `accuracy vs model size` 图。
  - 预留 MCU latency 记录区。
- `mcu_latency_template.csv`
  - 记录板端延迟测试结果的模板。
- `dataset/wanddata_5.json`
- `dataset/wanddata_6.json`
  - 这两份是后续 `4.3` 会用到的个人数据。由于 `group id = 15`，后续对应数字就是 `5` 和 `6`。

## 你需要操作的地方

### 1. 在 Colab 跑 notebook

打开：

- [4.2.ipynb](notebook/4.2.ipynb)

建议在 Google Colab 中执行 `Runtime -> Run all`。

它会完成：

- baseline 数据集下载
- 数据预处理
- 至少 8 个候选架构训练
- 导出每个架构的 `.keras`、float `.tflite`、quantized `.tflite`、`.cc`
- 生成结果表和 `accuracy vs model size` 图

### 2. 需要你在 Arduino 上做的部分

因为我当前不能直接连你的开发板，所以 `accuracy vs latency on the MCU` 这一步需要你操作：

1. 在 Colab 跑完 notebook 后，下载你要测试的几个模型对应的 `.cc` 文件。
2. 在 Arduino IDE 打开 Harvard TinyMLx 的 `magic_wand` 示例工程。
3. 每次替换工程里的模型数组文件为一个新的 `.cc` 模型。
4. 在调用 `interpreter->Invoke()` 前后加入 `micros()` 计时。
5. 串口运行多次，记录每个模型的平均 latency。
6. 把结果填入：
   - [mcu_latency_template.csv](mcu_latency_template.csv)

如果你愿意，我下一步可以继续直接帮你做：

- Arduino 端 `magic_wand` 延迟测量代码修改版
- 报告 4.1-4.2 的图表说明和文字草稿
- 根据你跑出来的真实结果，整理最终要交的表格和结论

## 作业要求摘要

根据 PDF，4.1-4.2 目前最关键的要求是：

- 先成功运行 baseline
- 至少训练 `5` 个不同模型架构
- 记录每个模型：
  - PC 端测试准确率
  - 量化后 `.tflite` 模型大小
- 绘制并分析：
  - accuracy vs model size
  - accuracy vs MCU latency

注意：

- PDF 明确说明 `cannot modify the current dataset split ratio`
- 后续 `4.3` 再使用个人数据，当前 `4.2` 先只用 baseline 的提供数据集
