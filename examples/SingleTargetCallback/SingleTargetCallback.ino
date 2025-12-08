/**
 * SingleTargetCallback.ino
 * 
 * Example that tracks only the closest/primary target.
 * Useful when you only need to follow one person.
 * 
 * The callback filters to report only the nearest valid target,
 * simplifying applications that don't need multi-person tracking.
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

// Store the closest target
RD03D_Target closestTarget;
bool hasTarget = false;

// Rate limiting
uint32_t lastOutput = 0;
const uint32_t OUTPUT_INTERVAL = 50;  // ms (~20 Hz)

/**
 * Callback - finds the closest target from all detected targets
 */
void onRadarFrame(RD03D_Target* targets, uint8_t count) {
    if (count == 0) {
        hasTarget = false;
        return;
    }
    
    // Find the closest valid target
    float minDistance = 99999;
    int closestIdx = -1;
    
    for (int i = 0; i < 3; i++) {
        if (targets[i].valid && targets[i].distance < minDistance) {
            minDistance = targets[i].distance;
            closestIdx = i;
        }
    }
    
    if (closestIdx >= 0) {
        closestTarget = targets[closestIdx];
        hasTarget = true;
    } else {
        hasTarget = false;
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n=== RD-03D Single Target Example ===\n");
    Serial.println("Tracking closest target only.\n");
    
    // Initialize radar
    radar.begin(Serial1, RADAR_RX_PIN, RADAR_TX_PIN);
    radar.onFrame(onRadarFrame);
    
    Serial.println("Radar ready.\n");
}

void loop() {
    // Process radar data
    radar.update();
    
    // Output at fixed rate
    if (millis() - lastOutput >= OUTPUT_INTERVAL) {
        lastOutput = millis();
        
        if (hasTarget) {
            Serial.printf("Target: %.1f cm @ %.1f° | X=%d Y=%d | Speed=%d cm/s\n",
                          closestTarget.distance,
                          closestTarget.angle,
                          closestTarget.x,
                          closestTarget.y,
                          closestTarget.speed);
        } else {
            // Uncomment to see when no target is detected
            // Serial.println("No target");
        }
    }
}
