/**
 * @file RD03D.h
 * @brief Arduino library for Ai-Thinker RD-03D 24GHz mmWave Radar
 * @author Nick Puckett
 * @version 1.0.0
 * 
 * This library provides an easy interface for reading multi-target
 * tracking data from the RD-03D radar sensor.
 * 
 * Features:
 * - Tracks up to 3 targets simultaneously
 * - Provides X, Y coordinates (mm), distance (cm), angle (degrees), speed (cm/s)
 * - Robust header-based frame parsing with timeout detection
 * - Callback support for new data events
 * 
 * Hardware: Connect radar TX to MCU RX pin, radar RX to MCU TX pin
 * Baud rate: 256000 (fixed by radar)
 */

#ifndef RD03D_H
#define RD03D_H

#include <Arduino.h>

// ============== CONFIGURATION ==============
#define RD03D_MAX_TARGETS      3
#define RD03D_FRAME_SIZE       30
#define RD03D_FRAME_HEADER_SIZE 4
#define RD03D_TARGET_DATA_SIZE 8
#define RD03D_BAUD_RATE        256000
#define RD03D_DEFAULT_TIMEOUT  100   // ms

// ============== TARGET DATA ==============
/**
 * @brief Structure holding data for a single tracked target
 */
struct RD03D_Target {
    int16_t x;           ///< X coordinate in mm (negative=left, positive=right)
    int16_t y;           ///< Y coordinate in mm (forward distance)
    int16_t speed;       ///< Speed in cm/s (negative=approaching, positive=receding)
    uint16_t distanceRaw;///< Raw distance resolution value from radar
    float distance;      ///< Calculated distance in cm (from X,Y)
    float angle;         ///< Calculated angle in degrees (from forward axis)
    bool valid;          ///< True if target is detected
    
    /**
     * @brief Clear target data
     */
    void clear() {
        x = 0;
        y = 0;
        speed = 0;
        distanceRaw = 0;
        distance = 0;
        angle = 0;
        valid = false;
    }
};

// ============== CALLBACK TYPES ==============
/**
 * @brief Callback function type for new frame events
 * @param targets Array of 3 target structures
 * @param count Number of valid targets (0-3)
 */
typedef void (*RD03D_FrameCallback)(RD03D_Target* targets, uint8_t count);

// ============== PARSER STATE ==============
enum RD03D_ParserState {
    RD03D_SYNC_HEADER,    ///< Looking for header AA FF 03 00
    RD03D_READ_DATA       ///< Reading target data and tail
};

// ============== MAIN CLASS ==============
/**
 * @brief Main class for RD-03D radar interface
 * 
 * Example usage:
 * @code
 * #include <RD03D.h>
 * 
 * RD03D radar;
 * 
 * void onRadarData(RD03D_Target* targets, uint8_t count) {
 *     for (int i = 0; i < count; i++) {
 *         Serial.printf("Target %d: %.1f cm @ %.1f°\n", 
 *                       i+1, targets[i].distance, targets[i].angle);
 *     }
 * }
 * 
 * void setup() {
 *     Serial.begin(115200);
 *     radar.begin(Serial1, 20, 21);  // Use Serial1, RX=20, TX=21
 *     radar.onFrame(onRadarData);
 * }
 * 
 * void loop() {
 *     radar.update();
 * }
 * @endcode
 */
class RD03D {
public:
    /**
     * @brief Constructor
     */
    RD03D();
    
    /**
     * @brief Initialize the radar with a HardwareSerial port
     * @param serial Reference to HardwareSerial (e.g., Serial1, Serial2)
     * @param rxPin RX pin number (receives data FROM radar)
     * @param txPin TX pin number (sends data TO radar)
     * @param rxBufferSize Size of receive buffer (default 512 for high baud rate)
     * @return true if initialization successful
     */
    bool begin(HardwareSerial& serial, int rxPin, int txPin, size_t rxBufferSize = 512);
    
    /**
     * @brief Process incoming radar data (call frequently in loop)
     * 
     * This method reads available bytes from the serial port and
     * processes them through the state machine. When a complete
     * valid frame is received, the callback is triggered.
     */
    void update();
    
    /**
     * @brief Enable multi-target detection mode
     * 
     * Sends the command to enable tracking of up to 3 targets.
     * This is called automatically by begin(), but can be called
     * again if needed.
     */
    void enableMultiTarget();
    
    /**
     * @brief Set callback for new frame data
     * @param callback Function to call when new data is available
     */
    void onFrame(RD03D_FrameCallback callback);
    
    /**
     * @brief Get target data by index
     * @param index Target index (0-2)
     * @return Pointer to target data, or nullptr if index invalid
     */
    RD03D_Target* getTarget(uint8_t index);
    
    /**
     * @brief Get all targets array
     * @return Pointer to array of 3 targets
     */
    RD03D_Target* getTargets();
    
    /**
     * @brief Get number of currently valid targets
     * @return Count of valid targets (0-3)
     */
    uint8_t getTargetCount();
    
    /**
     * @brief Get total frames received since begin()
     * @return Frame count
     */
    uint32_t getFrameCount();
    
    /**
     * @brief Get total parse errors since begin()
     * @return Error count
     */
    uint32_t getErrorCount();
    
    /**
     * @brief Set frame timeout in milliseconds
     * @param timeoutMs Timeout value (default 100ms)
     */
    void setTimeout(uint16_t timeoutMs);
    
    /**
     * @brief Check if radar is connected and sending data
     * @return true if frames received in last second
     */
    bool isConnected();

private:
    HardwareSerial* _serial;
    RD03D_Target _targets[RD03D_MAX_TARGETS];
    RD03D_FrameCallback _frameCallback;
    
    // Frame buffer and parser state
    uint8_t _frameBuf[RD03D_FRAME_SIZE];
    uint8_t _frameIdx;
    uint8_t _syncIdx;
    RD03D_ParserState _parserState;
    
    // Timing
    uint32_t _lastByteTime;
    uint32_t _lastFrameTime;
    uint16_t _timeoutMs;
    
    // Statistics
    uint32_t _frameCount;
    uint32_t _errorCount;
    
    // Frame constants
    static const uint8_t FRAME_HEADER[4];
    static const uint8_t MULTI_TARGET_CMD[12];
    
    // Private methods
    void processByte(uint8_t b);
    void parseTarget(uint8_t index, const uint8_t* data);
    void processFrame();
    void resetParser();
};

#endif // RD03D_H
