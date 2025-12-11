# RD03D Arduino Library

Arduino library for the Ai-Thinker RD-03D 24GHz mmWave Radar sensor with multi-target tracking.

![RD-03D Radar](https://img.shields.io/badge/Sensor-RD--03D-blue)
![Platform](https://img.shields.io/badge/Platform-ESP32-green)
![License](https://img.shields.io/badge/License-MIT-yellow)

## Features

- **Multi-target tracking**: Track up to 3 people/objects simultaneously
- **Rich data**: X/Y position (mm), distance (cm), angle (°), speed (cm/s)
- **Robust parsing**: Header-based state machine with timeout detection
- **Callback support**: Get notified when new data arrives
- **Easy API**: Simple begin/update pattern

## Hardware

- **Radar**: Ai-Thinker RD-03D (24GHz FMCW)
  - Detection range: 0-8 meters
  - Field of view: ±60° azimuth
  - Baud rate: 256000 (fixed)
- **Microcontroller**: ESP32 (any variant with hardware UART)

## Installation

### Arduino Library Manager (Recommended)
1. Open Arduino IDE
2. Go to **Sketch → Include Library → Manage Libraries**
3. Search for "RD03D"
4. Click **Install**

### Manual Installation
1. Download this repository as ZIP
2. In Arduino IDE: **Sketch → Include Library → Add .ZIP Library**
3. Select the downloaded ZIP file

## Wiring

| ESP32 Pin | RD-03D Pin | Description |
|-----------|------------|-------------|
| GPIO 20 (RX) | TX | Radar transmit → ESP receive |
| GPIO 21 (TX) | RX | ESP transmit → Radar receive |
| 3.3V | VCC | Power |
| GND | GND | Ground |

> **Note**: TX/RX are crossed - radar TX connects to ESP RX

## Quick Start

```cpp
#include <RD03D.h>

// Create the radar object
RD03D radar;

void setup() {
    Serial.begin(115200);
    
    // Start the radar on Serial1 with RX=GPIO20, TX=GPIO21
    radar.begin(Serial1, 20, 21);
}

void loop() {
    // Call update() to process incoming radar data
    radar.update();
    
    // Copy targets to local variables (avoids pointer syntax)
    RD03D_Target target1 = *radar.getTarget(0);
    RD03D_Target target2 = *radar.getTarget(1);
    RD03D_Target target3 = *radar.getTarget(2);
    
    // Check each target and print if valid
    if (target1.valid) {
        Serial.print("Target 1: ");
        Serial.print(target1.distance);
        Serial.print(" cm @ ");
        Serial.print(target1.angle);
        Serial.println(" degrees");
    }
    
    delay(100);
}
```

## API Reference

### Class: RD03D

#### Initialization

```cpp
bool begin(HardwareSerial& serial, int rxPin, int txPin, size_t rxBufferSize = 512)
```
Initialize radar on specified serial port and pins.

#### Main Loop

```cpp
void update()
```
Process incoming data. Call frequently in `loop()`.

#### Target Data

```cpp
RD03D_Target* getTarget(uint8_t index)  // Get single target (0-2)
RD03D_Target* getTargets()               // Get array of all 3 targets
uint8_t getTargetCount()                 // Number of valid targets
```

#### Callbacks

```cpp
void onFrame(RD03D_FrameCallback callback)
```
Set callback function for new data:
```cpp
void myCallback(RD03D_Target* targets, uint8_t count) {
    // Process targets here
}
radar.onFrame(myCallback);
```

#### Status

```cpp
bool isConnected()       // True if data received in last second
uint32_t getFrameCount() // Total frames received
uint32_t getErrorCount() // Parse errors
void setTimeout(uint16_t ms)  // Set frame timeout (default 100ms)
```

### Struct: RD03D_Target

| Field | Type | Description |
|-------|------|-------------|
| `x` | int16_t | X position in mm (negative=left, positive=right) |
| `y` | int16_t | Y position in mm (forward distance) |
| `speed` | int16_t | Speed in cm/s (negative=approaching, positive=receding) |
| `distance` | float | Calculated distance in cm |
| `angle` | float | Calculated angle in degrees from forward |
| `valid` | bool | True if target is detected |

## Examples

### BasicSerial
Simple example that prints target data to Serial Monitor.

### MultiTargetCallback
Uses callback function for event-driven processing.

### MultiTargetOSC
Sends data over Ethernet using OSC protocol for visualization in Processing, TouchDesigner, Max/MSP, etc.

## Processing Visualization

A companion Processing sketch is included in `extras/processing/` for visualizing radar data in real-time.

## RD-03D Protocol Details

The radar uses a proprietary binary protocol at 256000 baud. See the [protocol documentation](extras/protocol.md) for complete frame format details.

### Frame Format (30 bytes)
```
AA FF 03 00 [Target1:8] [Target2:8] [Target3:8] 55 CC
└─ Header ─┘                                    └Tail┘
```

### Target Data (8 bytes each)
| Bytes | Field | Encoding |
|-------|-------|----------|
| 0-1 | X | Sign-magnitude, bit 15 = positive |
| 2-3 | Y | Offset by 0x8000 |
| 4-5 | Speed | Sign-magnitude, bit 15 = positive |
| 6-7 | Distance | Raw resolution value |

## License

MIT License - see [LICENSE](LICENSE) for details.

## Contributing

Pull requests welcome! Please open an issue first to discuss proposed changes.

## Acknowledgments

- Protocol reverse-engineering based on Ai-Thinker documentation and testing
- Inspired by various mmWave radar projects in the maker community
