# Wiring Guide — ESP32-CAM Traffic Violation Detector

## Components Required

| Component              | Qty | Notes                          |
|------------------------|-----|--------------------------------|
| ESP32-CAM (AI-Thinker) | 1   | With OV2640 camera             |
| FTDI Programmer        | 1   | For uploading code             |
| IR Proximity Sensor    | 1   | Active LOW output              |
| Resistors (10kΩ)       | 2   | Pull-up for signal lines       |
| Jumper wires           | –   | Male-to-female                 |
| 5V Power Supply        | 1   | Min 2A recommended             |

---

## Wiring Diagram

```
                    ┌─────────────────────────────┐
                    │        ESP32-CAM             │
                    │      (AI-Thinker)            │
                    │                              │
  IR Sensor OUT ───►│ GPIO13         GPIO4 ───────►│ Flash LED (built-in)
  Traffic Signal ──►│ GPIO14                       │
  GND ─────────────►│ GND           3.3V ─────────►│ IR Sensor VCC
  5V ──────────────►│ 5V                           │
                    └─────────────────────────────┘
```

---

## FTDI Programmer (Upload Code Only)

| FTDI Pin | ESP32-CAM Pin |
|----------|---------------|
| GND      | GND           |
| VCC (5V) | 5V            |
| TX       | U0R (RX)      |
| RX       | U0T (TX)      |

> **Important:** Connect **GPIO0 → GND** before uploading code.  
> Disconnect this wire after upload to run normally.

---

## IR Proximity Sensor Wiring

| IR Sensor Pin | ESP32-CAM Pin |
|---------------|---------------|
| VCC           | 3.3V          |
| GND           | GND           |
| OUT           | GPIO13        |

> Output goes **LOW** when an object (vehicle) is detected.

---

## Traffic Signal Input

Connect a signal wire from your traffic light controller (5V logic):
- **HIGH (5V)** = Red Light Active
- **LOW (0V)**  = Green Light / Not Red

> Use a **voltage divider** (10kΩ + 10kΩ) if your signal is 5V to step it down to 3.3V for the ESP32.

---

## Power Supply

- Use a **5V 2A** power supply for stable operation.
- Do **not** power from FTDI during live deployment.

---

## System Flow

```
Vehicle enters junction
        │
  Is signal RED?
        │ YES
        ▼
  Start 5-second grace period
        │
  Is vehicle STILL present after 5s?
        │ YES
        ▼
  Capture photo (with flash)
        │
  Send to server via WiFi (HTTP POST)
        │
  Log in database + Dashboard updated
```
