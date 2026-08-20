# Disaster Early-Warning Mesh Network

A hybrid hardware + software system that monitors flood and landslide risk in rural, connectivity-poor areas using a LoRa sensor mesh, and delivers live, location-aware data to rescue teams through a fully offline local dashboard — no internet or cellular network required.

## Problem

Rural and hilly regions prone to floods and landslides often lack real-time monitoring. When disaster strikes, poor or absent network connectivity delays critical data from reaching rescue teams, costing precious response time exactly where it's needed most.

## Solution

Three ESP32 + LoRa nodes form a relay chain:

```
Node A (River zone)  --LoRa-->  Node B (Hilly zone)  --LoRa-->  Node C (Rescue dept)
   ultrasonic +                  rain + soil +                   local WiFi
   barometer                     vibration                       dashboard
```

- **Node A** sits at a river/flood-prone site, reading water level (ultrasonic) and pressure/temperature (barometer).
- **Node B** sits at a hilly/landslide-prone site, reading rainfall, soil moisture, and ground vibration. It also relays Node A's data onward — this is the mesh hop.
- **Node C** sits at the rescue department, receives the combined data from B, applies hardcoded threshold logic to compute a flood/landslide risk status, and hosts its **own WiFi access point** with a live webpage — the rescue team connects directly to it, no router or internet involved.

## Repository contents

| File | Description |
|---|---|
| `node_A_river.ino` | Firmware for Node A — river zone sensors |
| `node_B_landslide.ino` | Firmware for Node B — landslide zone sensors + relay logic |
| `node_C_rescue.ino` | Firmware for Node C — LoRa receiver + local web dashboard |
| `README.md` | This file |

## Hardware required

| Component | Qty | Used in |
|---|---|---|
| ESP32 dev board | 3 | A, B, C |
| SX1278 LoRa module (433 MHz) | 3 | A, B, C |
| Ultrasonic sensor (e.g. HC-SR04 / JSN-SR04T) | 1 | A |
| BMP280 barometer (I2C) | 1 | A |
| Rain sensor (analog) | 1 | B |
| Capacitive soil moisture sensor (analog) | 1 | B |
| Vibration sensor (e.g. SW-420, digital) | 1 | B |
| Voltage divider resistors (1kΩ + 2kΩ) | 1 set | A (ultrasonic ECHO line) |

## Wiring

### LoRa (identical on all 3 nodes)

| LoRa Pin | ESP32 GPIO |
|---|---|
| VCC | 3.3V |
| GND | GND |
| MISO | 19 |
| MOSI | 23 |
| SCK | 18 |
| NSS / CS | 5 |
| RESET | 14 |
| DIO0 | 2 |

### Node A extra wiring

| Sensor | Pin | ESP32 GPIO |
|---|---|---|
| BMP280 | SDA | 21 |
| BMP280 | SCL | 22 |
| Ultrasonic | TRIG | 27 |
| Ultrasonic | ECHO | 26 (via voltage divider — 5V to 3.3V) |

⚠️ Ultrasonic ECHO outputs 5V. ESP32 GPIO is not 5V tolerant — always wire ECHO through a voltage divider (1kΩ in series, 2kΩ to GND) before connecting to GPIO26.

### Node B extra wiring

| Sensor | Pin | ESP32 GPIO |
|---|---|---|
| Soil moisture | AOUT | 34 |
| Rain sensor | AOUT | 35 |
| Vibration | DOUT | 33 |

### Node C
No extra sensors — LoRa + built-in WiFi only.

## Software setup

1. Install **Arduino IDE** with ESP32 board support.
2. Install libraries via Library Manager:
   - `LoRa` by Sandeep Mistry
   - `Adafruit BMP280 Library` (and its dependency `Adafruit Unified Sensor`)
3. Flash each `.ino` file to its corresponding board.
4. All three boards must use the **same LoRa frequency** (`LoRa.begin(433E6)` by default) — change consistently across all three if your modules are 868/915 MHz instead.

## Calibration

- **Node A**: `readDistanceCm()` returns distance from the sensor down to the water surface. If mounted at a known height above a baseline, subtract to get true water level.
- **Node B**: `SOIL_AIR_VALUE`, `SOIL_WATER_VALUE`, `RAIN_DRY_VALUE`, `RAIN_WET_VALUE` are placeholders in the code — calibrate by reading each sensor fully dry and fully wet, then update the constants.
- **Node C**: risk thresholds (`waterCm`, `soilPercent`, `rainPercent`, `vibration`) are hardcoded estimates for the hackathon MVP — tune based on real deployment-site data.

## Running the system

1. Flash and power Node C first — open Serial Monitor to confirm it prints its access point IP (default `192.168.4.1`).
2. Flash and power Node B.
3. Flash and power Node A.
4. Within ~5–10 seconds, Serial output should show A sending, B receiving + relaying, and C receiving.
5. Connect a phone or laptop to WiFi network `DisasterMeshNode-C` (password `rescue123`), then open `http://192.168.4.1` in a browser to view the live dashboard.

## Communication protocol

Simple pipe-delimited text packets over LoRa:

```
Node A -> B:  A|water_cm:120.5|pressure_hpa:1008.3|temp_c:27.1
Node B -> C:  A|water_cm:120.5|pressure_hpa:1008.3|temp_c:27.1;B|rain:40|soil:65|vib:0
```

If Node B hasn't received data from A within 30 seconds, it forwards its own data with `A|status:no_data` in place of A's fields, so the chain never blocks on a single node going offline.

## Future scope

- Extend from 3 nodes to a full village-wide dynamic mesh (multi-hop, many nodes)
- Replace hardcoded thresholds with an ML model trained on real sensor data
- Add solar + battery power for permanent off-grid deployment
- Add GPS modules for automatic location tagging
- Build a full mobile/web app with alerts and historical trends
- Extend sensor types to cover additional hazards (fire, drought, air quality)

## License

Add your license here (e.g. MIT).

## Acknowledgements

- LoRa library by Sandeep Mistry
- Adafruit BMP280 / Unified Sensor libraries
- [Team members, mentors, and institution — add here]
