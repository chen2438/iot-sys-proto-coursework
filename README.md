# IoT System Prototyping Coursework

This repository contains the working files for the IoT System Prototyping coursework, including both design challenges, supporting lab material, generated figures, deployment assets, extracted assignment material, notebooks, datasets, and report drafts.

## Repository structure

- `coursework_assignment/`
  - Original coursework PDFs plus extracted plain text and per-page images used for requirement checking.
- `design-challenge-1/`
  - Zephyr-based implementation for the thermal monitoring and control system on the nRF54L15 DK.
- `design-challenge-2/`
  - Magic Wand TinyML work for coursework tasks `4.1`, `4.2`, `4.3`, and `5`, including trained model exports, MCU latency logs, plots, personal datasets, Colab notebooks, and report notes.
- `labs/`
  - Supporting lab templates and reference material used during development.

## Design Challenge 1

`design-challenge-1/` contains a multi-threaded Zephyr application built around the LM335 analogue temperature sensor on the nRF54L15 DK.

Implemented features visible in the code and local documentation include:

- dedicated ADC sampling, logic, reporting, BLE, and calibration threads
- `100 ms` periodic sampling and a `60 s` rolling average
- `NORMAL`, `WARNING`, `FAULT`, and `DRIFT` operating states
- LED alarm behaviour for warning and fault conditions
- BLE advertising with temperature and system state
- ADC fault detection and queue overrun tracking
- button-driven live calibration
- long-term drift detection with a full mode and a demo mode

Build entry point:

> You can build directly using the VS Code plugin.

```sh
west build -b nrf54l15dk/nrf54l15/cpuapp design-challenge-1
```

More detail is documented in:

- `design-challenge-1/README.md`
- `design-challenge-1/README_CN.md`

## Design Challenge 2

`design-challenge-2/` now covers all implemented TinyML coursework tasks in this repository:

- `4.1 Baseline`
- `4.2 Architecture Exploration`
- `4.3 Personalisation and Stability Analysis`
- `5 Extending the System with a New Gesture Class`

Available artefacts in the repository include:

- Colab notebooks for baseline, architecture exploration, personalisation, and class extension
- exported `.cc` TinyML models for multiple CNN variants
- an Arduino `magic_wand` project with MCU-side latency timing added around `interpreter->Invoke()`
- latency logs collected on-device for multiple candidate models
- CSV summary data for accuracy, model size, latency, personalisation, and class-extension results
- generated plots:
  - `accuracy_vs_model_size.png`
  - `accuracy_vs_mcu_latency.png`
  - `task_5_confusion_matrix.png`
- English and Chinese report materials

Key result files:

- `design-challenge-2/notebook/4.2.ipynb`
- `design-challenge-2/notebook/4.3.ipynb`
- `design-challenge-2/notebook/5.ipynb`
- `design-challenge-2/mcu_latency_template.csv`
- `design-challenge-2/notebook/task_4_3_comparison.csv`
- `design-challenge-2/notebook/task_5_summary.csv`
- `design-challenge-2/document/report_4_3_final.md`
- `design-challenge-2/document/report_5_final.md`
- `design-challenge-2/magic_wand/magic_wand.ino`

### Task 4.2 summary

Task `4.2` explores at least five CNN architectures and compares:

- PC-side classification accuracy
- quantized model size
- MCU inference latency

The repository includes trained architecture outputs, generated trade-off plots, deployment-ready `.cc` exports, and supporting report notes.

### Task 4.3 summary

For group `15`, the personal digits are `5` and `6`.

Final `4.3` results:

| Model variant | PC accuracy on original test (%) | PC accuracy on personalised test (%) | Quantized accuracy on original test (%) | Quantized accuracy on personalised test (%) | Personal repetition accuracy (%) | Personal consistency (%) |
|---|---:|---:|---:|---:|---:|---:|
| baseline_original | 93.73 | 91.96 | 93.82 | 91.87 | 52.17 | 53.08 |
| baseline_personalised | 91.91 | 90.65 | 91.82 | 90.30 | 95.65 | 96.16 |

These results show that adding small amounts of user-specific data greatly improved recognition stability for the target user’s gestures, while only slightly reducing performance on the original shared dataset.

### Task 5 summary

For group `15`, the new gesture class is letter `P`.

Final `Task 5` results:

| Metric | Value |
|---|---:|
| Baseline PC digit accuracy (%) | 96.00 |
| Extended PC digit accuracy (%) | 94.00 |
| Extended PC full 11-class accuracy (%) | 94.17 |
| Extended PC letter `P` test accuracy (%) | 100.00 |
| Extended repeated-letter accuracy (%) | 90.00 |
| Extended letter consistency (%) | 90.00 |

The confusion analysis shows no held-out test confusion involving `P`, so the added class was recognised cleanly. The main cost of extension was a small `2.00` percentage point drop in digit accuracy.

## Completion check

Based on the current repository contents, extracted assignment text, notebooks, result tables, and report files, the evidence supports the following status:

- `Design Challenge 1`: implemented and documented to a strong submission-ready level, including the extended `Live Calibration` and `Drift Detection` features.
- `Design Challenge 2 - 4.1 to 4.2`: completed with trained model variants, quantized deployment artefacts, MCU latency instrumentation, recorded latency logs, summary tables, plots, and report-ready conclusions.
- `Design Challenge 2 - 4.3`: completed with personalised data for digits `5` and `6`, comparison metrics, and a final write-up.
- `Design Challenge 2 - Task 5`: completed with the new class `P`, a trained `11-class` model, confusion analysis, consistency measurement, and a final write-up.

## Practical verdict

This repository now looks broadly submission-ready for:

- all of `Design Challenge 1`
- `Design Challenge 2` sections `4.1-4.3`
- `Design Challenge 2` `Task 5`

The main remaining risks are no longer missing implementations. They are presentation and submission details, for example:

- whether the final report wording aligns tightly with the rubric
- whether all figures and tables are referenced clearly in the write-up
- whether the embedded deployment notes are included where needed
- whether team and individual contributions are documented in the format expected by the module
