# Design Challenge 1

This file is maintained together with `README_CN.md`. Future updates should be
applied to both the English and Chinese versions.

This application implements the core requirements of Design Challenge 1 on the
nRF54L15 DK with an LM335 analogue temperature sensor wired as in Lab 2:

- `LM335 V+` to `P1.11 / AIN4`
- `LM335 V+` to `VDD (3.3V)` through `330 ohm`
- `LM335 GND` to board `GND`
- `LM335 ADJ` left unconnected

![image-20260417141829748](https://media.opennet.top/i/2026/04/17/z16w0p-0.png)

## Implemented requirements

- Four distinct threads:
  - ADC acquisition
  - Logic engine
  - Serial reporting
  - BLE communication
- Extension thread:
  - calibration input handler
- Precise `100 ms` sampling triggered from a kernel timer.
- One-minute rolling average over `600` samples.
- Deterministic `NORMAL`, `WARNING`, and `FAULT` state transitions.
- `50 ms` LED pulse in `WARNING`, solid LED in `FAULT`.
- BLE advertising with average temperature and system state.
- ADC fault detection for conversion failures and out-of-range voltages.
- Live calibration using board buttons with thread-safe threshold updates.
- Drift detection using minute-level Welford statistics without storing raw
  24-hour history.

## Thread structure

- `adc_sampling`: waits on a `100 ms` timer tick, reads the ADC, validates the
  voltage, and pushes a sample into a queue.
- `logic_engine`: consumes samples, updates the rolling average, drives the
  state machine, and controls the LED alarm behaviour.
- `reporting`: prints structured UART logs once per second.
- `communication`: updates BLE advertisements once per second and stops
  advertising while the system is in `FAULT`.
- `calibration`: receives button events and safely updates the warning
  threshold or drift-detection mode.

## Reliability features

- The acquisition path is decoupled from logging and BLE to minimise sampling
  jitter.
- Inter-thread communication uses a bounded message queue and mutex-protected
  shared state.
- ADC failures and queue overruns are counted and exposed in the UART log.
- The rolling average uses integer centi-degrees rather than floats in the
  history buffer to reduce SRAM use.
- Drift detection uses Welford online statistics over minute aggregates, so no
  24-hour sample array is required.

## BLE payload

The manufacturer specific payload contains:

- `int16` average temperature in centi-degrees Celsius, big-endian
- `uint8` system state:
  - `0 = NORMAL`
  - `1 = WARNING`
  - `2 = FAULT`
  - `3 = DRIFT`

## Calibration controls

- `SW0`: decrease warning threshold by `0.50 C`
- `SW1`: increase warning threshold by `0.50 C`
- `SW2`: reset warning threshold to `28.00 C` and reset drift tracking
- `SW3`: toggle drift detection mode between:
  - `24h` mode: requires `1440` controlled minutes
  - `demo` mode: requires `3` controlled minutes for rapid testing

## Build

Before building, set the board ADC reference to `3.3V` in Nordic Board
Configurator, as required in Lab 2.

Example build command:

```sh
west build -b nrf54l15dk/nrf54l15/cpuapp design-challenge-1
```

Flash with:

```sh
west flash
```

Open UART and verify output similar to:

```text
[12000 ms] Avg: 22.11 C | Raw: 22.25 C | MV: 2954 | Thr: 28.00 C | Mode: NORMAL | LED: OFF | DriftRef: *0.00 C | LastMin: 22.09 C | Ctrl: 2/1440 | Env: CTRL | ADC_ERR: 0 | Q_OVR: 0
```

## Testing

### Live calibration

1. Boot the board and confirm the UART log shows `Thr: 28.00 C`.
2. Press `SW0` several times. Each press should print `Calibration: threshold`
   and reduce the threshold by `0.50 C`.
3. Warm the sensor with your finger. Once the rolling average exceeds the new
   threshold, the mode should switch to `WARNING` and `LED` should report
   `BLINKING`.
4. Press `SW1` to raise the threshold above the current average; the mode
   should return to `NORMAL`.
5. Press `SW2` to restore the default `28.00 C` threshold.

### Drift detection

1. Press `SW3` once to enter `demo` mode. The UART log will report
   `Drift mode -> demo (3 controlled minute(s) required)`.
2. Leave the sensor untouched in a stable room environment for at least
   `3 minutes`. The log should eventually report
   `Drift baseline locked at ... [demo]`.
3. After the baseline is locked, warm the sensor and keep it elevated by more
   than `2.0 C` for at least one full minute while the environment is otherwise
   stable.
4. The mode should switch to `DRIFT`, and BLE state byte should become `3`.
5. Let the sensor return close to the baseline. Once the deviation falls below
   the clear threshold, the mode should leave `DRIFT`.
6. Press `SW3` again to return to real `24h` mode, or `SW2` to reset tracking.

### Fault handling

1. Disconnect the LM335 output or ground wire briefly.
2. The UART log should move to `FAULT`, `ADC_ERR` should increase, LED should
   become solid, and BLE advertising should stop.
