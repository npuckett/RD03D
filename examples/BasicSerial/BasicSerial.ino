/**
 * BasicSerial.ino
 * 
 * Simple example showing how to read RD-03D radar data
 * and output it over Serial for debugging.
 * 
 * Hardware:
 * - ESP32 (any variant with hardware UART)
 * - RD-03D radar connected to Serial1 (RX=20, TX=21)
 * 
 * Wiring:
 * - ESP32 GPIO20 (RX) -> Radar TX
 * - ESP32 GPIO21 (TX) -> Radar RX
 * - ESP32 3.3V -> Radar VCC
 * - ESP32 GND -> Radar GND
 */

#include <RD03D.h>

// Create radar instance
RD03D radar;

// Pin configuration - adjust for your board
#define RADAR_RX_PIN 20
#define RADAR_TX_PIN 21

void setup() {
    // Initialize debug serial
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n=== RD-03D Basic Serial Example ===\n");
    
    // Initialize radar on Serial1
    Serial.printf("Initializing radar on RX=%d, TX=%d...\n", RADAR_RX_PIN, RADAR_TX_PIN);
    
    if (radar.begin(Serial1, RADAR_RX_PIN, RADAR_TX_PIN)) {
        Serial.println("Radar initialized!");
    } else {
        Serial.println("Radar initialization failed!");
    }
    
    Serial.println("\nWaiting for targets...\n");
}

void loop() {
    // Process incoming radar data
    radar.update();
    
    // Print target data at ~10Hz
    static uint32_t lastPrint = 0;
    if (millis() - lastPrint > 100) {
        lastPrint = millis();
        
        uint8_t count = radar.getTargetCount();
        
        if (count > 0) {
            Serial.printf("--- %d target(s) detected ---\n", count);
            
            for (int i = 0; i < 3; i++) {
                RD03D_Target* t = radar.getTarget(i);
                if (t && t->valid) {
                    Serial.printf("  T%d: %.1f cm @ %.1f° | X=%d mm, Y=%d mm | Speed=%d cm/s\n",
                                  i + 1, 
                                  t->distance, 
                                  t->angle,
                                  t->x,
                                  t->y,
                                  t->speed);
                }
            }
            Serial.println();
        }
    }
    
    // Print status every 10 seconds
    static uint32_t lastStatus = 0;
    if (millis() - lastStatus > 10000) {
        lastStatus = millis();
        Serial.printf("[Status] Frames: %lu, Errors: %lu, Connected: %s\n",
                      radar.getFrameCount(),
                      radar.getErrorCount(),
                      radar.isConnected() ? "Yes" : "No");
    }
}
