#!/bin/zsh
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

if [ "$#" -ne 1 ]; then
  echo "Usage: ./use_model.sh <model-name>"
  echo "Available models:"
  echo "  tiny_2"
  echo "  mid_12_24"
  echo "  mid_16_24"
  echo "  mid_14_28"
  echo "  mid_14_24_24"
  echo "  baseline_cnn"
  exit 1
fi

MODEL_NAME="$1"
MODEL_SRC="${SCRIPT_DIR}/${MODEL_NAME}.cc"
MODEL_DST="${PROJECT_DIR}/magic_wand/magic_wand_model_data.cpp"

if [ ! -f "$MODEL_SRC" ]; then
  echo "Model file not found: $MODEL_SRC"
  exit 1
fi

cp "$MODEL_SRC" "$MODEL_DST"
echo "Copied ${MODEL_NAME}.cc -> ${MODEL_DST}"
echo "Next: reopen or verify the Arduino sketch, then upload to the board."
