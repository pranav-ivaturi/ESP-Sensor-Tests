# ESP32 Sensor Tests

A collection of sensor, actuator, encoder, and measurement test projects developed using **ESP32-WROOM-32** and **ESP-IDF**.

This repository contains individual test projects created while developing and testing hardware for embedded systems and sensor-integration projects.

---

## Repository Overview

The projects in this repository cover:

- Analog sensors and ADCs
- Temperature measurement
- Pressure measurement
- IMUs
- LiDAR / distance measurement
- Hall-effect sensing
- Rotary and magnetic encoders
- Wheel RPM and speed measurement
- Relays and optocouplers
- Motor/encoder testing

Each project is maintained as an independent ESP-IDF project so that sensors can be tested individually before being integrated into larger systems.

---

## Projects

| Project | Description |
|---|---|
| `3_Magnets` | Wheel RPM/speed measurement using three magnets |
| `4_Magnets` | Wheel RPM/speed measurement using four magnets |
| `ADS1115_Test` | Testing the ADS1115 external ADC |
| `ESP_Sensor_Tests_4_Magnets` | ESP32 sensor/encoder test using four magnets |
| `GY906_Test` | GY-906 infrared temperature sensor testing |
| `HallEffect_Test` | Hall-effect sensor testing |
| `Hydraulic_Pressure_Brake_Test` | Analog hydraulic brake pressure measurement |
| `ICM42688_Test` | ICM-42688-P IMU testing |
| `JohnsonMotor_Encoder_Test` | Motor and encoder testing |
| `LVMS_Test` | LVMS-related sensor/system testing |
| `Optocoupler_GPIO_Test` | GPIO testing using an optocoupler |
| `Optocoupler_Test` | HY-M154 / 817 optocoupler testing |
| `PT1000_Test` | PT1000 temperature sensor testing |
| `Relay_Test` | Relay control and GPIO testing |
| `RotaryEncoder_Test` | Rotary encoder testing |
| `VL53Lidar_Test` | VL53L0X LiDAR distance measurement |
| `Wheel_70m_LowRPM` | Very-low-speed wheel RPM/speed measurement |
| `Wheel_RPM_Test` | Wheel RPM and speed calculation |

---

## Hardware Platform

### Microcontroller

**ESP32-WROOM-32 DevKit**

### Development Environment

- ESP-IDF
- Visual Studio Code
- ESP-IDF VS Code Extension
- C / C++

---

## Common I²C Configuration

Several sensors in this repository use the ESP32's I²C interface.

| Signal | ESP32 GPIO |
|---|---:|
| SDA | GPIO 21 |
| SCL | GPIO 22 |
| Supply | 3.3 V |
| GND | Common GND |

Typical I²C addresses used during testing:

| Sensor | Address |
|---|---|
| BNO055 | `0x28` |
| VL53L0X | `0x29` |
| BMP280 | `0x76` / `0x77` |

---

## Sensor Hardware

The repository includes testing for hardware such as:

- ADS1115 ADC
- GY-906 / MLX90614 infrared temperature sensor
- PT1000 temperature sensor
- ICM-42688-P IMU
- BNO055 IMU
- BMP280 pressure/temperature sensor
- VL53L0X LiDAR
- Hall-effect sensors
- Rotary encoders
- Magnetic wheel encoders
- Hydraulic pressure transducers
- Relays
- Optocouplers

---

## Wheel Speed and RPM Testing

Several projects focus on measuring wheel rotation using magnets and sensors.

The encoder projects explore different numbers of magnets attached to a wheel, including:

- 3 magnets
- 4 magnets
- Earlier 2-magnet configurations

The measured rotation can be used to calculate:

**RPM**

```text
RPM = revolutions per minute
```

and wheel linear speed:

```text
v = π × D × RPM / 60
```

where:

- `v` = linear speed in m/s
- `D` = wheel diameter in metres
- `RPM` = wheel rotational speed

These calculations are useful for low-speed wheel and vehicle testing.

---

## Project Structure

Each sensor test is maintained as an independent ESP-IDF project.

Typical structure:

```text
Sensor_Test/
├── CMakeLists.txt
├── sdkconfig
└── main/
    ├── CMakeLists.txt
    └── main.c
```

Generated ESP-IDF build files are excluded from version control.

---

## Building a Project

Open a terminal inside the required project directory.

For example:

```bash
cd VL53Lidar_Test
```

Set the ESP32 target:

```bash
idf.py set-target esp32
```

Build the project:

```bash
idf.py build
```

Flash it to the ESP32:

```bash
idf.py flash
```

Monitor serial output:

```bash
idf.py monitor
```

Or build, flash, and monitor together:

```bash
idf.py build flash monitor
```

---

## Development Workflow

These projects are developed as individual hardware tests before being integrated into larger embedded systems.

Typical workflow:

```text
Sensor
   ↓
ESP32 wiring
   ↓
Individual sensor test
   ↓
Verify readings
   ↓
Calibration / filtering
   ↓
Performance testing
   ↓
Integration into larger system
```

---

## Git Workflow

The repository is maintained using Git and GitHub.

After making a meaningful change:

```bash
git add .
git commit -m "Describe the change"
git push
```

Example:

```bash
git add .
git commit -m "Improve VL53L0X distance measurement"
git push
```

---

## Purpose

This repository serves as a development and testing collection for embedded systems work involving the **ESP32**, sensors, measurement systems, encoders, and actuator interfaces.

The individual projects are intended to make hardware debugging and sensor integration easier by allowing each component to be tested independently before being incorporated into a larger system.

---

## Status

🚧 **Active development**

New sensor tests, hardware experiments, calibration improvements, and integration work will be added as development continues.
