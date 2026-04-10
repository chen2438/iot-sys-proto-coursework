# Magic Wand 模型切换与 Latency 测量说明

本说明整合了模型切换、Arduino 运行步骤和 MCU latency 测量方法，适用于 Design Challenge 2 的 `accuracy vs MCU latency` 部分。

## 目录结构

- 工程目录：
  [magic_wand](/Users/nanmener/Sync/UoB/TB2/03_IoT_System_Prototyping/coursework/design-challenge-2/magic_wand)
- 模型目录：
  [models](/Users/nanmener/Sync/UoB/TB2/03_IoT_System_Prototyping/coursework/design-challenge-2/models)
- 切换脚本：
  [use_model.sh](/Users/nanmener/Sync/UoB/TB2/03_IoT_System_Prototyping/coursework/design-challenge-2/models/use_model.sh)
- 延迟汇总表：
  [mcu_latency_template.csv](/Users/nanmener/Sync/UoB/TB2/03_IoT_System_Prototyping/coursework/design-challenge-2/mcu_latency_template.csv)

## 可切换模型

当前 `models` 目录中包含：

- `tiny_2`
- `mid_12_24`
- `mid_16_24`
- `mid_14_28`
- `mid_14_24_24`
- `baseline_cnn`

其中前 5 个是 4.2 报告里优先使用的模型，`baseline_cnn` 是额外保留的基线参考模型。

## 我对原工程文件做过的改动

为了测量板端推理时间，我修改了：

- [magic_wand.ino](/Users/nanmener/Sync/UoB/TB2/03_IoT_System_Prototyping/coursework/design-challenge-2/magic_wand/magic_wand.ino#L230)

原工程在调用 `interpreter->Invoke()` 时只执行推理，不输出延迟。我加入了基于 `micros()` 的计时代码，并通过串口输出：

```cpp
unsigned long start_time_micros = micros();
TfLiteStatus invoke_status = interpreter->Invoke();
unsigned long end_time_micros = micros();
unsigned long inference_latency_micros = end_time_micros - start_time_micros;

Serial.print("Latency: ");
Serial.print(inference_latency_micros);
Serial.println(" us");
```

也就是说，和原始工程相比，新增的是：

- 推理前记录开始时间
- 推理后记录结束时间
- 计算单次 inference latency
- 串口打印 `Latency: xxxx us`

除了这一段 latency 测量逻辑外，原有 `magic_wand` 推理流程没有被改动。

## 每次测试一个模型的完整流程

### 1. 切换模型

在终端进入 `models` 目录：

```bash
cd /Users/nanmener/Sync/UoB/TB2/03_IoT_System_Prototyping/coursework/design-challenge-2/models
chmod +x use_model.sh
```

例如切到 `tiny_2`：

```bash
./use_model.sh tiny_2
```

切到其他模型：

```bash
./use_model.sh mid_12_24
./use_model.sh mid_16_24
./use_model.sh mid_14_28
./use_model.sh mid_14_24_24
./use_model.sh baseline_cnn
```

这个脚本会把所选模型复制到：

- [magic_wand_model_data.cpp](/Users/nanmener/Sync/UoB/TB2/03_IoT_System_Prototyping/coursework/design-challenge-2/magic_wand/magic_wand_model_data.cpp)

## 2. 打开 Arduino 工程

在 Arduino IDE 打开：

- [magic_wand.ino](/Users/nanmener/Sync/UoB/TB2/03_IoT_System_Prototyping/coursework/design-challenge-2/magic_wand/magic_wand.ino)

## 3. 上传到开发板

- 开发板选择 `Arduino Nano 33 BLE`
- 端口选择当前连接的板子
- 点击上传

## 4. 打开串口监视器

保持示例工程原本的波特率设置，做手势后会看到类似输出：

```text
Latency: 12345 us
Found 5 (87)
```

说明：

- `Latency: 12345 us` 是这一次推理的板端延迟
- `Found 5 (87)` 表示预测结果和对应分数
- 括号里的分数不是 latency，不需要写入 latency 表

## 5. 如何记录 latency

建议每个模型至少测 10 次，记录：

- `latency_us_avg`
- `latency_us_min`
- `latency_us_max`

如果只是为了 4.2，优先测试这 5 个模型即可：

1. `tiny_2`
2. `mid_12_24`
3. `mid_16_24`
4. `mid_14_28`
5. `mid_14_24_24`

然后把结果填回：

- [mcu_latency_template.csv](/Users/nanmener/Sync/UoB/TB2/03_IoT_System_Prototyping/coursework/design-challenge-2/mcu_latency_template.csv)

## 推荐写法

在报告中可以这样描述部署与测试方法：

`A CNN-based neural network was trained in Google Colab, quantized to TensorFlow Lite format, and deployed on the Arduino Nano 33 BLE Sense using TensorFlow Lite Micro. MCU inference latency was measured by inserting timing code around interpreter->Invoke() in the magic_wand sketch and printing the elapsed time through the serial monitor.`
