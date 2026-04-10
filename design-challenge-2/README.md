# Design Challenge 2

This directory contains the coursework material for `Design Challenge 2` of IoT System Prototyping.

The work currently covers:

- `4.1 Baseline`
- `4.2 Architecture Exploration`
- `4.3 Personalisation and Stability Analysis`

## Folder overview

- `notebook/`
  - Colab notebooks for the baseline, architecture exploration, and personalisation tasks.
- `dataset/`
  - Personal datasets for group `15`, corresponding to digits `5` and `6`.
- `models/`
  - Exported `.cc` model files used for Arduino / TensorFlow Lite Micro deployment.
- `latency_log/`
  - MCU latency logs collected for selected `4.2` models.
- `document/`
  - Report drafts and supporting write-ups.

## Task 4.1

The baseline Magic Wand workflow was run successfully using the provided digit dataset and the original train / validation / test split. This baseline is required before the assessed tasks, but it is not itself assessed.

Reference notebook:

- `notebook/Baseline.ipynb`

## Task 4.2

Task `4.2` explores different CNN architectures and analyses the trade-off between:

- classification accuracy
- quantized model size
- MCU inference latency

The repository already contains:

- trained architecture outputs
- exported `.cc` model files
- `accuracy_vs_model_size.png`
- `accuracy_vs_mcu_latency.png`
- MCU latency records
- report-ready summary material

Main files:

- `notebook/4.2.ipynb`
- `mcu_latency_template.csv`
- `document/report_4_1_4_2_draft_final.md`

## Task 4.3

Task `4.3` studies the effect of personal data on both accuracy and prediction stability.

For group `15`, the personal digits are:

- `5`
- `6`

Personal dataset files:

- `dataset/wanddata_5.json`
- `dataset/wanddata_6.json`

The personalised experiment compares:

- a baseline model trained on the original dataset
- a personalised model trained on the original dataset plus the personal recordings

### Final 4.3 result summary

| Model variant | PC accuracy on original test (%) | PC accuracy on personalised test (%) | Quantized accuracy on original test (%) | Quantized accuracy on personalised test (%) | Personal repetition accuracy (%) | Personal consistency (%) |
|---|---:|---:|---:|---:|---:|---:|
| baseline_original | 93.73 | 91.96 | 93.82 | 91.87 | 52.17 | 53.08 |
| baseline_personalised | 91.91 | 90.65 | 91.82 | 90.30 | 95.65 | 96.16 |

These results show that adding personal data greatly improves recognition of the user’s own repeated gestures while only slightly reducing performance on the original shared dataset.

Main files:

- `notebook/4.3.ipynb`
- `notebook/task_4_3_comparison.csv`
- `document/report_4_3_final.md`

## Practical conclusion

`4.2` shows the trade-off between architecture complexity and embedded deployment cost, while `4.3` shows that personalisation can dramatically improve user-specific stability and accuracy. Together, these tasks provide both a system-level and user-level evaluation of TinyML gesture recognition on the Arduino Nano 33 BLE Sense.
