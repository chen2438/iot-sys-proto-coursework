# Design Challenge 2 Final Write-up for Task 5

## 5. Extending the System: Adding a New Gesture Class

### Experimental setup

- Group ID: `15`
- Assigned new class: `P`
- Personal dataset file:
  - `design-challenge-2/dataset/wanddata_P.json`
- Personal dataset size:
  - total `P` samples: `30`
- Baseline model:
  - `10-class` CNN for digits `0-9`
- Extended model:
  - `11-class` CNN for digits `0-9` plus `P`
- Training platform:
  - Google Colab
- Deployment format:
  - TensorFlow Lite integer quantization
- Dataset split ratio:
  - unchanged from the provided notebook

### Method

The original Magic Wand digit dataset was first used to rebuild the baseline `10-class` recognizer. A separate personal dataset for letter `P` was then split using the same train, validation, and test ratio as the original coursework workflow. The `P` samples were merged into the corresponding digit subsets to form an `11-class` extended dataset.

The same baseline CNN architecture was used for both models so that the comparison focused on the effect of class extension rather than architecture changes. Both models were exported as quantized TensorFlow Lite models, and the extended model was additionally exported as a `.cc` array for embedded deployment. If this extended model is deployed on the Arduino sketch, the label mapping in `magic_wand.ino` must also be updated from `10` labels to `11`, with `P` appended to the label list.

### Main result table

| Metric | Value |
|---|---:|
| Baseline PC digit accuracy (%) | `96.00` |
| Baseline quantized digit accuracy (%) | `96.00` |
| Extended PC digit accuracy (%) | `94.00` |
| Extended quantized digit accuracy (%) | `94.00` |
| Extended PC full 11-class accuracy (%) | `94.17` |
| Extended quantized full 11-class accuracy (%) | `94.17` |
| Extended PC letter `P` test accuracy (%) | `100.00` |
| Extended quantized letter `P` test accuracy (%) | `100.00` |
| Extended repeated-letter accuracy (%) | `90.00` |
| Extended repeated-letter consistency (%) | `90.00` |

### Impact on digit recognition accuracy

Using the same held-out digit test subset, the baseline `10-class` model achieved `96.00%` PC accuracy, while the extended `11-class` model achieved `94.00%`. This means that adding the new class `P` reduced digit accuracy by `2.00` percentage points.

This is a relatively small drop, suggesting that the recognizer scales reasonably well when one extra class is added. The system does pay a modest accuracy cost for the new functionality, but the original digit recognizer remains strong overall.

### Recognition performance for the new letter

The extended model recognised the held-out `P` test samples perfectly, achieving `100.00%` accuracy on both the PC model and the quantized model. On the full repeated-recording set of `30` personal `P` samples, the model achieved `90.00%` accuracy and `90.00%` consistency.

The consistency metric was defined as:

`consistency = modal prediction count / total repeated recordings`

For the repeated `P` recordings, the modal prediction was also `P`, with `27` out of `30` samples predicted as the same class. This indicates that the extended model recognised the new class in a stable way across repeated attempts.

### Confusion analysis

The confusion matrix shows that the new class `P` was recognised cleanly in the held-out test subset. There were no confusion pairs involving `P` in `task_5_p_confusions.csv`, meaning:

- no non-`P` sample was misclassified as `P`
- no held-out `P` sample was misclassified as a digit

The remaining confusion in the `11-class` model occurred only between some digit classes:

- digit `0` was sometimes predicted as `8`
- digit `2` was sometimes predicted as `7`
- digit `3` was sometimes predicted as `5`
- digit `5` was sometimes predicted as `7`

So, the main effect of class extension was not confusion between `P` and the digits. Instead, the slight drop in digit accuracy appears to come from a small redistribution of model capacity across the original digit classes.

### Short conclusion

Task `5` successfully extended the gesture recognizer from `10` classes to `11` classes by adding letter `P`. The final results show that the extended model still maintained strong overall performance, with only a small digit-accuracy drop from `96.00%` to `94.00%`, while recognising the new class `P` very well. The held-out `P` samples were classified perfectly, and the repeated-recording consistency for `P` reached `90.00%`.

Overall, this suggests that the embedded gesture-recognition system scales well when a single new class is introduced. The added functionality is achieved at a relatively low cost to the original digit recognition performance.
