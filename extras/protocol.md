# RD-03D Protocol Documentation

Complete documentation of the Ai-Thinker RD-03D 24GHz mmWave radar binary protocol for multi-target detection mode.

## Communication Settings

| Parameter | Value |
|-----------|-------|
| Baud Rate | 256000 |
| Data Bits | 8 |
| Parity | None |
| Stop Bits | 1 |

## Enabling Multi-Target Mode

By default, the radar operates in single-target mode. Send this 12-byte command to enable multi-target tracking:

```
FD FC FB FA 02 00 90 00 04 03 02 01
```

### Command Breakdown

| Bytes | Value | Description |
|-------|-------|-------------|
| 0-3 | `FD FC FB FA` | Command preamble (start marker) |
| 4-5 | `02 00` | Payload length: 2 bytes (little-endian) |
| 6-7 | `90 00` | Command ID: Enable multi-target detection |
| 8-11 | `04 03 02 01` | Command postamble (end marker) |

## Data Frame Format

After enabling multi-target mode, the radar continuously outputs 30-byte frames:

```
┌───────────────────────────────────────────────────────────────────────────────┐
│ Offset:  0   1   2   3 │ 4-11      │ 12-19     │ 20-27     │ 28  29          │
│          ──────────────────────────────────────────────────────               │
│          Header        │ Target 1  │ Target 2  │ Target 3  │ Tail            │
│          AA FF 03 00   │ (8 bytes) │ (8 bytes) │ (8 bytes) │ 55 CC           │
└───────────────────────────────────────────────────────────────────────────────┘
```

### Header (4 bytes)

| Byte | Value | Description |
|------|-------|-------------|
| 0 | `0xAA` | Frame start marker |
| 1 | `0xFF` | Frame start marker |
| 2 | `0x03` | Number of targets in frame (3) |
| 3 | `0x00` | Reserved |

### Target Data Block (8 bytes each)

Each of the 3 target slots contains 8 bytes of data:

```
┌───────────────────────────────────────────────────────────────────┐
│ Offset: +0   +1  │ +2   +3  │ +4   +5  │ +6   +7                  │
│         ──────────────────────────────────────                    │
│         X coord   │ Y coord  │ Speed    │ Distance                │
│         (2 bytes) │ (2 bytes)│ (2 bytes)│ (2 bytes)               │
└───────────────────────────────────────────────────────────────────┘
```

### Tail (2 bytes)

| Byte | Value | Description |
|------|-------|-------------|
| 28 | `0x55` | Frame end marker |
| 29 | `0xCC` | Frame end marker |

---

## Field Encoding Details

All multi-byte values are **little-endian** (low byte first).

### X Coordinate (bytes 0-1)

- **Unit**: Millimeters from sensor centerline
- **Range**: Approximately ±8000 mm (±8 meters)
- **Encoding**: Sign-magnitude with inverted sign bit

```
Bit:  15  14  13  12  11  10  9   8   7   6   5   4   3   2   1   0
      ├───┼───────────────────────────────────────────────────────┤
      Sign                    Magnitude (0-32767)
      
      Bit 15 = 1  →  POSITIVE (target is to the RIGHT of sensor)
      Bit 15 = 0  →  NEGATIVE (target is to the LEFT of sensor)
```

**Decoding Algorithm:**
```cpp
uint16_t raw = byte[0] | (byte[1] << 8);  // Read little-endian
int16_t x = (raw & 0x7FFF);               // Extract magnitude (bits 0-14)
if (!(raw & 0x8000)) {                    // If bit 15 is NOT set
    x = -x;                               // Value is negative
}
```

**Examples:**
| Raw Bytes (hex) | Raw Value | Bit 15 | Decoded Value |
|-----------------|-----------|--------|---------------|
| `E8 83` | 0x83E8 (33768) | 1 (set) | +1000 mm |
| `E8 03` | 0x03E8 (1000) | 0 (clear) | -1000 mm |
| `00 80` | 0x8000 (32768) | 1 (set) | 0 mm |
| `00 00` | 0x0000 (0) | 0 | 0 mm (no target) |

---

### Y Coordinate (bytes 2-3)

- **Unit**: Millimeters from sensor (forward/radial distance)
- **Range**: 0 to approximately 8000 mm
- **Encoding**: Unsigned with 0x8000 (32768) offset

```
Actual Y = Raw Value - 0x8000
```

**Decoding Algorithm:**
```cpp
uint16_t raw = byte[2] | (byte[3] << 8);  // Read little-endian
int16_t y = (int16_t)(raw - 0x8000);      // Subtract offset
```

**Examples:**
| Raw Bytes (hex) | Raw Value | Calculation | Decoded Value |
|-----------------|-----------|-------------|---------------|
| `00 80` | 0x8000 (32768) | 32768 - 32768 | 0 mm |
| `E8 83` | 0x83E8 (33768) | 33768 - 32768 | +1000 mm |
| `D0 87` | 0x87D0 (34768) | 34768 - 32768 | +2000 mm |
| `00 A0` | 0xA000 (40960) | 40960 - 32768 | +8192 mm |

---

### Speed (bytes 4-5)

- **Unit**: Centimeters per second
- **Range**: Approximately ±100+ cm/s
- **Encoding**: Same sign-magnitude as X coordinate

```
Bit 15 = 1  →  POSITIVE speed (target moving AWAY from sensor)
Bit 15 = 0  →  NEGATIVE speed (target moving TOWARD sensor)
```

**Decoding Algorithm:**
```cpp
uint16_t raw = byte[4] | (byte[5] << 8);
int16_t speed = (raw & 0x7FFF);
if (!(raw & 0x8000)) {
    speed = -speed;
}
```

**Examples:**
| Raw Bytes | Bit 15 | Decoded | Meaning |
|-----------|--------|---------|---------|
| `0A 80` | set | +10 cm/s | Moving away at 10 cm/s |
| `0A 00` | clear | -10 cm/s | Approaching at 10 cm/s |
| `32 80` | set | +50 cm/s | Moving away at 50 cm/s |

---

### Distance Resolution (bytes 6-7)

- **Unit**: Internal radar resolution value
- **Usage**: Typically not used directly

The actual distance is better calculated from X and Y:

```cpp
float distance_cm = sqrtf(x*x + y*y) / 10.0f;  // X,Y in mm, result in cm
float angle_deg = atan2f(x, y) * 180.0f / PI;  // Angle from forward axis
```

---

## Invalid/Empty Targets

When fewer than 3 targets are detected, unused slots contain all zeros:

```
00 00 00 00 00 00 00 00
```

**Validity Check:**
```cpp
bool isValid = (raw_x != 0 || raw_y != 0);
```

---

## Complete Frame Example

### Raw Frame (30 bytes, hex):
```
AA FF 03 00  E8 83 D0 87 0A 80 00 00  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  55 CC
└─ Header ─┘ └────── Target 1 ──────┘ └────── Target 2 ──────┘ └────── Target 3 ──────┘ └Tail┘
```

### Decoded:

| Field | Raw Bytes | Raw Value | Decoded |
|-------|-----------|-----------|---------|
| **Target 1** | | | |
| X | `E8 83` | 0x83E8 | +1000 mm (1m right) |
| Y | `D0 87` | 0x87D0 | +2000 mm (2m forward) |
| Speed | `0A 80` | 0x800A | +10 cm/s (moving away) |
| Distance | (calculated) | √(1000²+2000²)/10 | 223.6 cm |
| Angle | (calculated) | atan2(1000,2000) | 26.6° |
| **Target 2** | `00 00...` | all zeros | Not detected |
| **Target 3** | `00 00...` | all zeros | Not detected |

---

## Frame Timing

| Parameter | Value |
|-----------|-------|
| Baud rate | 256000 bps |
| Bits per byte | 10 (8 data + 1 start + 1 stop) |
| Time per byte | ~39 μs |
| Time per frame | ~1.2 ms (30 bytes) |
| Typical frame rate | 20-50 Hz |

---

## Parsing Best Practices

1. **Sync on header, not tail**: Look for `AA FF 03 00` to start frame, don't just scan for `55 CC`

2. **Use a state machine**: More robust than simple buffer scanning
   - State 1: SYNC_HEADER - Look for `AA FF 03 00`
   - State 2: READ_DATA - Collect 26 remaining bytes
   - Validate tail, process, reset

3. **Handle timeouts**: If no bytes received for >100ms during a frame, reset parser

4. **Use large RX buffer**: Set to 512+ bytes to handle high baud rate

5. **Rate limit output**: Radar sends faster than most applications need; throttle to 20-50 Hz

---

## Coordinate System

```
                    +Y (forward)
                      ↑
                      │
                      │
         -X (left) ←──┼──→ +X (right)
                      │
                      │
                   [SENSOR]
```

- **Origin**: Sensor location
- **+Y axis**: Forward (direction sensor is facing)
- **+X axis**: Right of sensor
- **-X axis**: Left of sensor
- **Angle**: Measured from +Y axis, positive clockwise
