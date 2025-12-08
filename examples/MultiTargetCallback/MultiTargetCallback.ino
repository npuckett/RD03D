/**
 * MultiTargetCallback.ino
 * 
 * Example using callback function for radar data.
 * This approach is useful when you want to process
 * data as soon as it arrives.
 * 
 * Hardware:
 * - ESP32 (any variant with hardware UART)
 * - RD-03D radar connected to Serial1 (RX=20, TX=21)
 */

#include <RD03D.h>

// Create radar instance
RD03D radar;

// Pin configuration
#define RADAR_RX_PIN 20
#define RADAR_TX_PIN 21

// Rate limiting for output
uint32_t lastOutput = 0;
const uint32_t OUTPUT_INTERVAL = 50;  // ms

/**
 * Callback function - called automatically when new frame received
 */
void onRadarFrame(RD03D_Target* targets, uint8_t count) {
    // Rate limit output to avoid flooding serial
    if (millis() - lastOutput < OUTPUT_INTERVAL) return;
    lastOutput = millis();
    
    // Output as CSV format for easy parsing
    // Format: timestamp,count,t1_x,t1_y,t1_dist,t1_angle,t1_speed,...
    
    Serial.print(millis());
    Serial.print(",");
    Serial.print(count);
    
    for (int i = 0; i < 3; i++) {
        Serial.print(",");
        if (targets[i].valid) {
            Serial.print(targets[i].x);
            Serial.print(",");
            Serial.print(targets[i].y);
            Serial.print(",");
            Serial.print(targets[i].distance, 1);
            Serial.print(",");
            Serial.print(targets[i].angle, 1);
            Serial.print(",");
            Serial.print(targets[i].speed);
        } else {
            Serial.print("0,0,0,0,0");
        }
    }
    Serial.println();
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("# RD-03D Callback Example");
    Serial.println("# Format: timestamp,count,t1_x,t1_y,t1_dist,t1_angle,t1_speed,t2_...,t3_...");
    Serial.println();
    
    // Initialize radar
    radar.begin(Serial1, RADAR_RX_PIN, RADAR_TX_PIN);
    
    // Set callback function
    radar.onFrame(onRadarFrame);
    
    Serial.println("# Radar ready, streaming data...");
}

void loop() {
    // Just call update - callback handles the rest
    radar.update();
}
