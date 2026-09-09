# Exoskeleton Sensor & Actuator Control System

A prototype/simulation of a single-joint lower-limb exoskeleton control system using an ESP32, joint-position sensor, TB6612FNG motor driver, and DC geared motor.

The system converts joint-position sensor readings into joint angle, classifies the angle into safety zones, and generates direction/PWM commands for actuator control. Hard-limit conditions are latched as faults until manually reset.

> **Note:** This is a prototype/simulation for internship evaluation. It is not a medical device and has not been validated for human use.

---

## System Architecture

```text
Joint Position Sensor
        │
        ▼
   ESP32 ADC (GPIO34)
        │
        ▼
   Angle Conversion
     0–4095 → 0–120°
        │
        ▼
 Zone Classification
   & Control Logic
        │
        ▼
 PWM + Direction
        │
        ▼
   TB6612FNG
   Motor Driver
        │
        ▼
 DC Geared Motor
   (Actuator)
````

See [`figures/block_diagram.png`](figures/block_diagram.png) for the complete system-level architecture.

---

## Hardware Design

### Joint Position Sensor

* 10 kΩ linear potentiometer
* Connected to ESP32 ADC GPIO34
* 12-bit ADC range: `0–4095`
* Prototype joint-angle range: `0–120°`

Angle conversion:

```cpp
jointAngle = ((float)potRaw / 4095.0) * 120.0;
```

### Microcontroller

* ESP32-WROOM-32 / ESP32 DevKit
* ADC input: GPIO34
* PWM output: GPIO21
* Motor direction:

  * GPIO22 → AIN1
  * GPIO23 → AIN2
* Standby:

  * GPIO16 → STBY

### Motor Driver

TB6612FNG H-bridge motor driver.

Control signals:

| ESP32  | TB6612FNG |
| ------ | --------- |
| GPIO21 | PWM       |
| GPIO22 | AIN1      |
| GPIO23 | AIN2      |
| GPIO16 | STBY      |

### Motor

A small brushed DC geared motor is used as the proposed actuator.

The Wokwi simulation does **not** mechanically simulate the motor. It verifies sensor input and the generated control outputs.

---

## Control Logic

The joint angle is divided into five operating zones:

| Joint Angle | Zone         | Action                      |
| ----------- | ------------ | --------------------------- |
| `< 5°`      | `FAULT_LOW`  | Latched fault + motor brake |
| `5–15°`     | `LIMIT_LOW`  | Forward PWM command         |
| `15–105°`   | `SAFE`       | Motor stopped               |
| `105–115°`  | `LIMIT_HIGH` | Reverse PWM command         |
| `> 115°`    | `FAULT_HIGH` | Latched fault + motor brake |

The fault state remains active until a manual reset command is received through the serial monitor:

```text
r
```

---

## Motor Commands

### STOP

```text
AIN1 = 0
AIN2 = 0
PWM  = 0
```

### FORWARD

```text
AIN1 = 0
AIN2 = 1
PWM  = 80–180
```

### REVERSE

```text
AIN1 = 1
AIN2 = 0
PWM  = 80–180
```

### BRAKE

```text
AIN1 = 1
AIN2 = 1
PWM  = 255
```

---

## Repository Contents

```text
.
├── diagram.json
├── sketch.ino
├── figures/
│   ├── block_diagram.png
│   ├── serial_fault_high.png
│   ├── serial_fault_low.png
│   ├── serial_limit_high.png
│   ├── serial_limit_low.png
│   ├── serial_safe.png
│   └── wokwi_circuit.png
└── report/
    └── Exoskeleton_Final_Submission_Report.pdf
```

### Important Files

* [`sketch.ino`](sketch.ino) — ESP32 control firmware
* [`diagram.json`](diagram.json) — Wokwi simulation configuration
* [`report/Exoskeleton_Final_Submission_Report.pdf`](report/Exoskeleton_Final_Submission_Report.pdf) — final technical report
* [`figures/block_diagram.png`](figures/block_diagram.png) — system architecture
* [`figures/wokwi_circuit.png`](figures/wokwi_circuit.png) — simulation circuit
* `figures/serial_*.png` — simulation test evidence

---

## Simulation

The Wokwi model is used to verify:

* Potentiometer/ADC input acquisition
* ADC-to-angle conversion
* Safety-zone classification
* PWM generation
* Motor direction command generation
* Fault detection
* Fault latching
* Manual fault reset

The potentiometer is manually adjusted in Wokwi to emulate joint movement.

---

## Verified Test Cases

| Test        | ADC  | Angle  | Result               |
| ----------- | ---- | ------ | -------------------- |
| SAFE        | 1973 | 57.8°  | STOP                 |
| LIMIT_LOW   | 304  | 8.9°   | FORWARD, PWM 150     |
| LIMIT_HIGH  | 3611 | 105.8° | REVERSE, PWM 80      |
| FAULT_LOW   | 44   | 1.3°   | BRAKE, latched fault |
| FAULT_HIGH  | 4095 | 120.0° | BRAKE, latched fault |
| After reset | —    | 57.8°  | SAFE, STOP           |

Detailed screenshots are available in the `figures/` directory.

---

## Safety Features

The prototype control logic includes:

* Software-defined joint-angle limits
* Latched fault state
* Brake command during fault
* Manual reset requirement
* Defined safe operating region

For a physical exoskeleton, additional protection would be required, including mechanical end stops, emergency-stop circuitry, current protection, thermal protection, appropriate power regulation, and hardware-level safety validation.

These physical protections were **not experimentally validated in this simulation**.

---

## Power Design Basis

A proposed physical implementation may use:

* 2S Li-ion battery
* Nominal battery voltage: 7.4 V
* Full-charge voltage: 8.4 V
* Regulated motor-driver supply
* Separate regulated logic supply for the ESP32

Example design calculation:

```text
Battery energy = 2.0 Ah × 7.4 V
              = 14.8 Wh

At an assumed average power of 5 W:

Runtime ≈ 14.8 Wh / 5 W
        ≈ 2.96 hours
```

This runtime is a **design estimate based on an assumed average power consumption**, not a measured result.

---

## Limitations

This project is a simulation/prototype and has several limitations:

* No mechanical coupling between the potentiometer and motor in Wokwi
* Physical motor movement was not experimentally verified
* Emergency-stop hardware was not tested
* Mechanical end stops were not tested
* Motor current and thermal behavior were not measured
* No clinical or human-subject validation was performed

Physical hardware validation would be required before any load-bearing or human-worn application.

---

## Tools

* Arduino/C++
* ESP32
* Wokwi
* Git/GitHub

---

## Author

**Akshaj C P**

B.Tech Electronics & Communication Engineering
TKM College of Engineering

