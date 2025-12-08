/**
 * @file RD03D.cpp
 * @brief Implementation of RD03D library
 */

#include "RD03D.h"

// Frame header: AA FF 03 00
const uint8_t RD03D::FRAME_HEADER[4] = {0xAA, 0xFF, 0x03, 0x00};

// Multi-target detection command
const uint8_t RD03D::MULTI_TARGET_CMD[12] = {
    0xFD, 0xFC, 0xFB, 0xFA,  // Preamble
    0x02, 0x00,              // Length
    0x90, 0x00,              // Command: multi-target mode
    0x04, 0x03, 0x02, 0x01   // Postamble
};

RD03D::RD03D() {
    _serial = nullptr;
    _frameCallback = nullptr;
    _frameIdx = 0;
    _syncIdx = 0;
    _parserState = RD03D_SYNC_HEADER;
    _lastByteTime = 0;
    _lastFrameTime = 0;
    _timeoutMs = RD03D_DEFAULT_TIMEOUT;
    _frameCount = 0;
    _errorCount = 0;
    
    // Clear all targets
    for (uint8_t i = 0; i < RD03D_MAX_TARGETS; i++) {
        _targets[i].clear();
    }
}

bool RD03D::begin(HardwareSerial& serial, int rxPin, int txPin, size_t rxBufferSize) {
    _serial = &serial;
    
    // Configure serial port
    _serial->setRxBufferSize(rxBufferSize);
    _serial->begin(RD03D_BAUD_RATE, SERIAL_8N1, rxPin, txPin);
    
    delay(100);
    
    // Clear any stale data
    while (_serial->available()) {
        _serial->read();
    }
    
    // Enable multi-target mode
    enableMultiTarget();
    
    // Reset parser state
    resetParser();
    _lastByteTime = millis();
    _lastFrameTime = millis();
    
    return true;
}

void RD03D::enableMultiTarget() {
    if (_serial) {
        _serial->write(MULTI_TARGET_CMD, sizeof(MULTI_TARGET_CMD));
        delay(100);
    }
}

void RD03D::update() {
    if (!_serial) return;
    
    uint32_t now = millis();
    
    // Check for frame timeout (partial frame stuck in buffer)
    if (_parserState != RD03D_SYNC_HEADER && (now - _lastByteTime > _timeoutMs)) {
        _errorCount++;
        resetParser();
    }
    
    // Process all available bytes
    while (_serial->available()) {
        uint8_t b = _serial->read();
        processByte(b);
    }
}

void RD03D::processByte(uint8_t b) {
    _lastByteTime = millis();
    
    switch (_parserState) {
        case RD03D_SYNC_HEADER:
            // Look for header bytes AA FF 03 00
            if (b == FRAME_HEADER[_syncIdx]) {
                _frameBuf[_syncIdx] = b;
                _syncIdx++;
                if (_syncIdx >= RD03D_FRAME_HEADER_SIZE) {
                    // Header found, now read data
                    _parserState = RD03D_READ_DATA;
                    _frameIdx = RD03D_FRAME_HEADER_SIZE;
                }
            } else if (b == FRAME_HEADER[0]) {
                // Could be start of new header
                _syncIdx = 1;
                _frameBuf[0] = b;
            } else {
                _syncIdx = 0;
            }
            break;
            
        case RD03D_READ_DATA:
            _frameBuf[_frameIdx++] = b;
            // Frame is 30 bytes: header(4) + 3*target(24) + tail(2)
            if (_frameIdx >= RD03D_FRAME_SIZE) {
                // Check tail bytes
                if (_frameBuf[28] == 0x55 && _frameBuf[29] == 0xCC) {
                    processFrame();
                } else {
                    _errorCount++;
                }
                resetParser();
            }
            break;
            
        default:
            resetParser();
            break;
    }
}

void RD03D::parseTarget(uint8_t index, const uint8_t* data) {
    if (index >= RD03D_MAX_TARGETS) return;
    
    RD03D_Target& t = _targets[index];
    
    // Read raw 16-bit values (little endian)
    uint16_t raw_x = data[0] | (data[1] << 8);
    uint16_t raw_y = data[2] | (data[3] << 8);
    uint16_t raw_speed = data[4] | (data[5] << 8);
    uint16_t raw_dist = data[6] | (data[7] << 8);
    
    // Check if target is valid (non-zero data)
    t.valid = (raw_x != 0 || raw_y != 0);
    
    if (!t.valid) {
        t.clear();
        return;
    }
    
    // X coordinate: bit 15 is sign indicator
    // Per datasheet: bit 15 = 1 means positive, bit 15 = 0 means negative
    int16_t x_val = (raw_x & 0x7FFF);  // Get magnitude (lower 15 bits)
    if (!(raw_x & 0x8000)) {           // If bit 15 is NOT set, value is negative
        x_val = -x_val;
    }
    t.x = x_val;
    
    // Y coordinate: offset by 0x8000 (always positive, forward direction)
    t.y = (int16_t)(raw_y - 0x8000);
    
    // Speed: bit 15 indicates direction
    // Positive = moving away, Negative = approaching
    int16_t spd_val = (raw_speed & 0x7FFF);
    if (!(raw_speed & 0x8000)) {
        spd_val = -spd_val;
    }
    t.speed = spd_val;
    
    // Distance resolution value
    t.distanceRaw = raw_dist;
    
    // Calculate distance in cm from X,Y coordinates (which are in mm)
    float x_mm = (float)t.x;
    float y_mm = (float)t.y;
    t.distance = sqrtf(x_mm * x_mm + y_mm * y_mm) / 10.0f;
    
    // Calculate angle: atan2(x, y) gives angle from forward (Y) axis
    // Positive X = right, Negative X = left
    t.angle = atan2f(x_mm, y_mm) * 180.0f / PI;
}

void RD03D::processFrame() {
    _frameCount++;
    _lastFrameTime = millis();
    
    // Parse all 3 targets from the frame buffer
    // Target 1: bytes 4-11, Target 2: bytes 12-19, Target 3: bytes 20-27
    parseTarget(0, &_frameBuf[4]);
    parseTarget(1, &_frameBuf[12]);
    parseTarget(2, &_frameBuf[20]);
    
    // Call user callback if set
    if (_frameCallback) {
        _frameCallback(_targets, getTargetCount());
    }
}

void RD03D::resetParser() {
    _parserState = RD03D_SYNC_HEADER;
    _syncIdx = 0;
    _frameIdx = 0;
}

void RD03D::onFrame(RD03D_FrameCallback callback) {
    _frameCallback = callback;
}

RD03D_Target* RD03D::getTarget(uint8_t index) {
    if (index >= RD03D_MAX_TARGETS) return nullptr;
    return &_targets[index];
}

RD03D_Target* RD03D::getTargets() {
    return _targets;
}

uint8_t RD03D::getTargetCount() {
    uint8_t count = 0;
    for (uint8_t i = 0; i < RD03D_MAX_TARGETS; i++) {
        if (_targets[i].valid) count++;
    }
    return count;
}

uint32_t RD03D::getFrameCount() {
    return _frameCount;
}

uint32_t RD03D::getErrorCount() {
    return _errorCount;
}

void RD03D::setTimeout(uint16_t timeoutMs) {
    _timeoutMs = timeoutMs;
}

bool RD03D::isConnected() {
    return (millis() - _lastFrameTime) < 1000;
}
