#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "Usage: $0 <serial-port>"
  echo "Detect the CYD port first with: arduino-cli board list"
  exit 64
fi

PORT="$1"
FQBN="esp32:esp32:esp32:UploadSpeed=115200"
BUILD_DIR="${TMPDIR:-/tmp}/cyd-analog-pin-scanner-build"

TFT_FLAGS=(
  "-DUSER_SETUP_LOADED=1"
  "-DILI9341_DRIVER=1"
  "-DTFT_WIDTH=240"
  "-DTFT_HEIGHT=320"
  "-DTFT_MISO=12"
  "-DTFT_MOSI=13"
  "-DTFT_SCLK=14"
  "-DTFT_CS=15"
  "-DTFT_DC=2"
  "-DTFT_RST=-1"
  "-DTFT_BL=21"
  "-DTFT_BACKLIGHT_ON=HIGH"
  "-DLOAD_GLCD=1"
  "-DSPI_FREQUENCY=40000000"
  "-DSPI_READ_FREQUENCY=16000000"
)

rm -rf "${BUILD_DIR}"

arduino-cli compile \
  --fqbn "${FQBN}" \
  --build-path "${BUILD_DIR}" \
  --build-property "compiler.cpp.extra_flags=${TFT_FLAGS[*]}" \
  AnalogPinScanner

arduino-cli upload \
  --fqbn "${FQBN}" \
  --port "${PORT}" \
  --input-dir "${BUILD_DIR}"
