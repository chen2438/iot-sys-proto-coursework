# Design Challenge 2 Report Stub (Tasks 4.1-4.2)

## 4.1 Baseline

We first ran the provided Magic Wand baseline successfully in Google Colab using the original digit dataset and the original dataset split ratio. The baseline uses IMU data from the Arduino Nano 33 BLE Sense and a CNN model deployed with TensorFlow Lite Micro.

This baseline was used only as the starting point for later experiments and was not itself assessed.

## 4.2 Architecture Exploration

### Experimental setup

- Dataset: baseline Magic Wand digit dataset
- Split ratio: unchanged from the provided notebook
- Input size: `32 x 32 x 3`
- Platform for training and PC-side evaluation: Google Colab
- Deployment target for latency measurement: Arduino Nano 33 BLE Sense

### Model variants

Include a table with at least 5 architectures:

| Model | Conv layers | Kernel size | Dense units | PC accuracy (%) | Quantized model size (bytes) | MCU latency (us) |
|---|---|---:|---:|---:|---:|---:|
| tiny_1 | 8 | 3 | 16 |  |  |  |
| tiny_2 | 8-16 | 3 | 16 |  |  |  |
| small_3 | 16-16-24 | 3 | 24 |  |  |  |
| baseline_like | 16-32-32 | 3 | 32 |  |  |  |
| wide_3 | 24-48-48 | 3 | 32 |  |  |  |

### Accuracy vs model size

Insert the plot generated from the notebook here.

Suggested discussion points:

- Smaller models reduced memory footprint but often had lower classification accuracy.
- Increasing the number of filters or layers generally improved accuracy, but the model size also increased.
- The best model was not necessarily the largest one; beyond a certain point, extra complexity gave only limited gains.

### Accuracy vs MCU latency

Insert the Arduino latency plot here.

Suggested discussion points:

- Model latency generally increased with model size and architectural complexity.
- Models with more channels or deeper stacks caused more on-device computation.
- The most practical embedded model is the one that balances accuracy, memory footprint, and latency rather than maximizing only one metric.

### Short conclusion

The architecture exploration demonstrates the trade-off between recognition performance and embedded deployment cost. For TinyML systems, choosing an architecture requires balancing accuracy with quantized model size and MCU inference latency.
