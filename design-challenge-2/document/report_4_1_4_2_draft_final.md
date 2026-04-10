# Design Challenge 2 Draft for Tasks 4.1-4.2

## 4.1 Baseline

We first ran the provided Magic Wand notebook successfully in Google Colab using the original digit dataset and the original train/validation/test split. This step confirmed that the provided workflow could be executed correctly before starting the architecture exploration. The baseline itself was not assessed directly in this section.

## 4.2 Architecture Exploration

### Experimental setup

- Dataset: provided Magic Wand digit dataset
- Data split: unchanged from the provided notebook
- Input shape: `32 x 32 x 3`
- Training and PC-side evaluation: Google Colab
- Deployment target for latency measurement: Arduino Nano 33 BLE Sense
- Quantization: TensorFlow Lite integer quantization as used in the notebook workflow

### Selected architectures

The five selected CNN architectures all produced quantized accuracies within the required 70%-90% band, which made them suitable for trade-off analysis.

| Model | Conv layers | Quantized test accuracy (%) | Quantized model size (bytes) | Average MCU latency (us) |
|---|---|---:|---:|---:|
| tiny_2 | 8-16 | 74.09 | 5536 | 41006.36 |
| mid_12_24 | 12-24 | 81.00 | 7544 | 61851.45 |
| mid_16_24 | 16-24 | 73.00 | 8624 | 75026.65 |
| mid_14_28 | 14-28 | 71.64 | 8744 | 73696.20 |
| mid_14_24_24 | 14-24-24 | 87.18 | 14608 | 69963.86 |

### Accuracy vs model size

Use this figure:

`design-challenge-2/accuracy_vs_model_size.png`

The model-size results show a clear overall trend: larger quantized models generally achieved higher accuracy. The smallest model, `tiny_2`, required only 5536 bytes but achieved 74.09% quantized test accuracy. Increasing the model capacity to `mid_12_24` raised the quantized accuracy to 81.00% at a cost of 7544 bytes. The best point among the selected models was `mid_14_24_24`, which reached 87.18% with a model size of 14608 bytes.

This indicates that increasing the number of convolution filters improved recognition performance, but the memory footprint also increased. Therefore, model size must be considered together with accuracy when selecting a deployable TinyML model.

### Accuracy vs MCU latency

Use this figure:

`design-challenge-2/accuracy_vs_mcu_latency.png`

The latency results also reveal a trade-off. `tiny_2` was the fastest model, with an average latency of 41006.36 us, but it also had the lowest accuracy among the selected models. `mid_12_24` provided a stronger balance, increasing quantized accuracy to 81.00% while keeping latency at 61851.45 us. The most interesting result was `mid_14_24_24`, which achieved the highest quantized accuracy, 87.18%, while still keeping average latency below both `mid_16_24` and `mid_14_28`.

This shows that the best embedded model is not necessarily the smallest or the fastest, but the one that offers the best balance between recognition accuracy, memory usage, and inference latency.

### Short conclusion

The architecture exploration demonstrates the expected TinyML trade-off between recognition performance and embedded deployment cost. For this task, `mid_14_24_24` provided the best overall balance among the evaluated models because it delivered the highest quantized accuracy in the target range while keeping both model size and latency within a practical level for the Arduino Nano 33 BLE Sense.
