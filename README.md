# ESP32-C3 Distance Sensor with OLED Display

This is a study project that demonstrates the use of a VL53L0X distance sensor with an ESP32-C3 OLED Shield from Abrobot, powered by the Zephyr RTOS.

## Overview

The project reads distance measurements from a VL53L0X Time-of-Flight sensor and displays them on an OLED screen. It implements a smoothing algorithm to prevent sudden changes in readings and provides both numeric and visual (progress bar) feedback.

https://github.com/user-attachments/assets/1d50550c-6bab-42f2-ba55-5e22bd498b45

### Features

- Real-time distance measurements using VL53L0X sensor
- Smooth readings using Exponential Moving Average (EMA)
- Spike detection and filtering using a 3-measurement window
- Visual feedback through:
  - Numeric distance display
  - Dynamic progress bar
  - Out-of-range indication
- I2C communication with both OLED and sensor

## Hardware Requirements

- ESP32-C3 OLED Shield (Abrobot)
- VL53L0X Time-of-Flight Distance Sensor
- Appropriate connections for I2C communication

## Software Components

- Zephyr RTOS
- LVGL (Light and Versatile Graphics Library)
- Device drivers:
  - SSD1306 OLED Display (SH1106 controller)
  - VL53L0X Distance Sensor
- Custom smoothing algorithm:
  - EMA with configurable alpha
  - Spike detection using a 3-measurement moving window

## Building and Running

This project is built using the Zephyr build system. Make sure you have the Zephyr SDK installed and properly set up.

```bash
west build -p auto -b esp32c3_042_oled --shield abrobot_sh1106_72x40 ../path/this_project
west flash
```

## Links

- [Zephyr RTOS](https://www.zephyrproject.org/)
- [LVGL](https://lvgl.io/)
- [VL53L0X Distance Sensor](https://www.st.com/en/imaging-and-photonics-solutions/vl53l0x.html)
- [ESP32-C3 OLED Shield](https://docs.zephyrproject.org/latest/boards/shields/abrobot_esp32c3_oled/doc/index.html)
