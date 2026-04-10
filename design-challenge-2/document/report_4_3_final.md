# Design Challenge 2 Final Write-up for Task 4.3

## 4.3 Personalisation and Stability Analysis

### Experimental setup

- Group ID: `15`
- Personal digits: `5` and `6`
- Personal dataset files:
  - `design-challenge-2/dataset/wanddata_5.json`
  - `design-challenge-2/dataset/wanddata_6.json`
- Personal dataset size:
  - digit `5`: `20` samples
  - digit `6`: `26` samples
  - total personal samples: `46`
- Baseline architecture: original Magic Wand baseline CNN
- Training platform: Google Colab
- Deployment format: TensorFlow Lite integer quantization
- Dataset split ratio: unchanged from the provided notebook

### Method

The original Magic Wand digit dataset was first used to train the baseline CNN model. A personalised version of the model was then created by mixing the personal gesture recordings for digits `5` and `6` into the original dataset and retraining the same baseline CNN architecture. This ensured that the comparison focused on the effect of personal data rather than architectural changes.

The original dataset kept the same train / validation / test ratio as the provided notebook. The personal samples were split separately with the same ratio and then merged into the corresponding train, validation, and test subsets of the original dataset. The rasterized image generation step followed the same baseline-style workflow with data augmentation so that the personalisation experiment remained comparable to the earlier baseline and architecture-exploration work.

### Accuracy comparison

The final comparison results are shown below.

| Model variant | PC accuracy on original test (%) | PC accuracy on personalised test (%) | Quantized accuracy on original test (%) | Quantized accuracy on personalised test (%) | Personal repetition accuracy (%) | Personal consistency (%) |
|---|---:|---:|---:|---:|---:|---:|
| baseline_original | 93.73 | 91.96 | 93.82 | 91.87 | 52.17 | 53.08 |
| baseline_personalised | 91.91 | 90.65 | 91.82 | 90.30 | 95.65 | 96.16 |

These results show that the baseline model already performs strongly on the shared dataset, achieving `93.73%` PC accuracy and `93.82%` quantized accuracy on the original test set. After adding the personal recordings, the personalised model showed only a small drop on the original shared dataset, falling to `91.91%` PC accuracy and `91.82%` quantized accuracy.

However, the personalised model improved substantially on the user-specific repeated gesture recordings. Personal repetition accuracy increased from `52.17%` to `95.65%`, and the consistency score increased from `53.08%` to `96.16%`. This indicates that the personalised model adapted very effectively to the user’s own gesture style.

### Consistency metric

The consistency metric was defined as:

`consistency = modal prediction count / total repeated recordings`

For each of the personal digits `5` and `6`, all repeated recordings were evaluated. The modal predicted label was identified, and the proportion of samples matching that modal prediction was computed. The overall consistency score was then defined as the mean of the per-digit consistency values.

This metric is appropriate for Task `4.3` because it measures prediction stability across repeated attempts of the same intended gesture, rather than only reporting general classification accuracy.

### Interpretation

The key result is that personalisation greatly improved recognition of the target user’s own gestures while only slightly reducing performance on the original shared dataset. The drop in shared-dataset accuracy was small, around `1.8` percentage points on the PC-side original test set, but the gain on the user’s repeated personal gestures was very large, exceeding `43` percentage points in personal repetition accuracy.

This suggests a clear trade-off:

- the baseline model generalises slightly better to the original shared data
- the personalised model is far better for recognising the specific user’s own drawing style

For on-device use, where the main goal is reliable recognition of the user’s personal gestures, the personalised model is the stronger choice.

### Short conclusion

Task `4.3` demonstrates that adding user-specific data can significantly improve both personal-gesture accuracy and prediction stability. In this experiment, the personalised model greatly outperformed the baseline model on repeated recordings of digits `5` and `6`, while only slightly reducing accuracy on the original shared dataset. Therefore, the personalisation step was successful and shows that modest amounts of personal data can substantially improve embedded gesture recognition for a specific user.
