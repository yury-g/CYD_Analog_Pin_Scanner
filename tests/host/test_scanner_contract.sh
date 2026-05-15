#!/usr/bin/env bash
set -euo pipefail

SKETCH="${1:-AnalogPinScanner/AnalogPinScanner.ino}"
README="${README:-README.md}"
FLASH_SCRIPT="${FLASH_SCRIPT:-flash-cyd.sh}"

require_literal() {
  local file="$1"
  local needle="$2"
  local description="$3"

  if ! rg -q --fixed-strings -- "${needle}" "${file}"; then
    echo "FAIL: ${description}"
    echo "Missing literal: ${needle}"
    echo "In file: ${file}"
    exit 1
  fi
}

require_literal "${SKETCH}" "AnalogPin pins[] = {" "scanner should define a fixed analog pin table"
require_literal "${SKETCH}" "{\"IO32\", 32" "scanner should include IO32"
require_literal "${SKETCH}" "{\"IO33\", 33" "scanner should include IO33"
require_literal "${SKETCH}" "{\"IO34\", 34" "scanner should include IO34"
require_literal "${SKETCH}" "{\"IO35\", 35" "scanner should include IO35"
require_literal "${SKETCH}" "{\"IO36\", 36" "scanner should include IO36"
require_literal "${SKETCH}" "{\"IO39\", 39" "scanner should include IO39"
require_literal "${SKETCH}" "analogReadResolution(12);" "scanner should use 12-bit ADC values for discovery"
require_literal "${SKETCH}" "analogSetAttenuation(ADC_11db);" "scanner should use a wide ADC input range"
require_literal "${SKETCH}" "Yellow row = most movement." "screen should explain the hot row"
require_literal "${SKETCH}" "PulseSensor signal is usually a wiggling mid-range value." "screen should describe the useful signal shape"
require_literal "${SKETCH}" "tft.print(\"rail\");" "scanner should flag railed readings"
require_literal "${README}" 'This scanner explains the `Why GPIO 35` note' "README should connect scanner to the main CYD firmware"
require_literal "${FLASH_SCRIPT}" "Detect the CYD port first with: arduino-cli board list" "flash script should require fresh port detection"
require_literal "${FLASH_SCRIPT}" "--build-property \"compiler.cpp.extra_flags=\${TFT_FLAGS[*]}\"" "flash script should keep TFT flags local"

echo "Analog pin scanner contract checks passed"
