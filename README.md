# CYD Analog Pin Scanner

Small diagnostic sketch for the ESP32 CYD / Cheap Yellow Display.

Use this when you have a sensor or loose signal wire and need to find which exposed CYD analog-capable GPIO is actually receiving it. It was originally used to confirm that a PulseSensor signal appeared on `IO35`, not the initially expected `GPIO 36`.

## What It Does

- Reads raw ADC values directly, without PulseSensorPlayground or other sensor libraries.
- Scans `IO32`, `IO33`, `IO34`, `IO35`, `IO36`, and `IO39`.
- Shows each pin as a live bar on the CYD screen.
- Tracks recent min/max movement for each pin.
- Highlights the pin with the most movement in yellow.
- Prints live serial readings at `115200` baud.

## Flash It

Detect the port fresh:

```bash
arduino-cli board list
```

Then flash using the detected USB serial port:

```bash
bash ./flash-cyd.sh /dev/cu.usbserial-3110
```

The script compiles with the CYD `TFT_eSPI` flags locally. You do not need to edit global `TFT_eSPI/User_Setup.h`.

## How To Use It

1. Flash the sketch.
2. Connect the signal wire you want to identify.
3. Touch or move the sensor/input.
4. Watch for the yellow row and the largest `d####` movement value.

For a PulseSensor, the useful signal is usually a wiggling mid-range ADC value, not a pin stuck near `0` or `4095`.

## Hardware Notes

- Backlight: `GPIO 21`, set `OUTPUT/HIGH`
- Display: ILI9341 320x240 CYD
- ADC resolution: 12-bit for scanning, so values run `0..4095`
- Scanner pins: `32`, `33`, `34`, `35`, `36`, `39`

## Relationship To CYD App Launcher

This scanner explains the `Why GPIO 35` note in [CYD_App_Launcher](https://github.com/yury-g/CYD_App_Launcher). It is intentionally separate so it can be flashed as a quick diagnostic tool without touching the main PulseSensor dashboard firmware.

This is an educational hardware diagnostic sketch, not a medical tool.
