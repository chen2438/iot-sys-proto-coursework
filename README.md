# IoT System Prototyping Coursework

This repository contains the working files for the IoT System Prototyping coursework, including both design challenges, supporting lab material, generated figures, deployment assets, and report drafts.

## Repository structure

- `Coursework Assignment.pdf`
  - Original coursework brief.
- `design-challenge-1/`
  - Zephyr-based implementation for the thermal monitoring and control system on the nRF54L15 DK.
- `design-challenge-2/`
  - Magic Wand TinyML work for coursework tasks `4.1` and `4.2`, including trained model exports, MCU latency logs, plots, and report notes.
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

```sh
west build -b nrf54l15dk/nrf54l15/cpuapp design-challenge-1
```

More detail is documented in:

- `design-challenge-1/README.md`
- `design-challenge-1/README_CN.md`

## Design Challenge 2

`design-challenge-2/` currently covers coursework task `4.1 Baseline` and task `4.2 Architecture Exploration`.

Available artefacts in the repository include:

- Colab notebooks for baseline and architecture exploration
- exported `.cc` TinyML models for multiple CNN variants
- an Arduino `magic_wand` project with MCU-side latency timing added around `interpreter->Invoke()`
- latency logs collected on-device for multiple candidate models
- CSV summary data for accuracy, model size, and latency
- generated plots:
  - `accuracy_vs_model_size.png`
  - `accuracy_vs_mcu_latency.png`
- English and Chinese report drafts for tasks `4.1-4.2`

Key result files:

- `design-challenge-2/notebook/4.2.ipynb`
- `design-challenge-2/mcu_latency_template.csv`
- `design-challenge-2/magic_wand/magic_wand.ino`
- `design-challenge-2/document/report_4_1_4_2_draft_final.md`

## Completion check

Based on the current repository contents, the evidence supports the following status:

- `Design Challenge 1`: very likely complete in implementation terms. The codebase and challenge README indicate that the main functional requirements have been implemented, including reliability, BLE communication, calibration, and drift handling.
- `Design Challenge 2 - 4.1 to 4.2`: very likely complete. The repository already contains trained model variants, quantized deployment artefacts, MCU latency instrumentation, recorded latency logs, summary tables, plots, and report-ready conclusions.

Important note:

This completion check is based on the local repository evidence and direct code inspection. I was able to inspect the project files thoroughly, but I was not able to extract the coursework PDF into clean searchable text with the tools available in this environment. Because of that, the statements above should be read as a careful repository-based audit rather than a strict line-by-line compliance certification against every sentence in the PDF brief.

## Practical verdict

If your question is whether this repository already looks submission-ready for:

- all of `Design Challenge 1`
- `Design Challenge 2` sections `4.1-4.2`

then the answer is: **yes, broadly speaking it does**.

The only remaining risk is not missing code, but whether the final submitted report text matches the exact wording and evidence expected by the coursework brief.
